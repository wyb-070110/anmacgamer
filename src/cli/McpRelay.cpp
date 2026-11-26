#include "McpRelay.h"

#include <gs/base/JsonNesting.h>

#include <nlohmann/json.hpp>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace gs::cli {

namespace {

using Json = nlohmann::json;

constexpr int kSuccess = 0;
constexpr int kProblems = 1;
constexpr int kUsage = 2;
constexpr std::size_t kChunkBytes = 64 * 1024;
constexpr int kBusyAttempts = 5;
constexpr std::size_t kMostLineBytes = std::size_t{32} << 20;
constexpr int kRefused = -32001;
constexpr std::string_view kOffText =
    "anmacgamer is not accepting AI connections. Open anmacgamer, turn on Settings > AI connection (MCP) > Allow AI "
    "to connect through MCP, then start this server again.";

struct Shared {
    std::mutex mutex;
    long long outstanding = 0;
    bool inputEnded = false;
};

// GameScrcpy listens with a QLocalServer: on macOS a Unix socket named in the temporary folder ($TMPDIR, as Qt's
// QDir::tempPath), or the path GS_MCP_PIPE gives when it is absolute.
std::string socketPath()
{
    const char* given = std::getenv("GS_MCP_PIPE");
    const std::string chosen = given != nullptr && *given != '\0' ? std::string(given) : std::string("GameScrcpy-MCP");
    if (chosen.starts_with('/')) {
        return chosen;
    }
    std::string folder = std::getenv("TMPDIR") != nullptr ? std::string(std::getenv("TMPDIR")) : std::string("/tmp");
    while (folder.size() > 1 && folder.ends_with('/')) {
        folder.pop_back();
    }
    return folder + "/" + chosen;
}

int connectTo(const std::string& path)
{
    sockaddr_un address{};
    if (path.size() >= sizeof(address.sun_path)) {
        return -1;
    }
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
    for (int attempt = 0; attempt < kBusyAttempts; ++attempt) {
        const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            return -1;
        }
        const int on = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
        if (::connect(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0) {
            return fd;
        }
        const int error = errno;
        ::close(fd);
        if (error != EAGAIN && error != ECONNREFUSED) {
            break;
        }
        if (error == ECONNREFUSED) {
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // source-rule-allow R9: retry backoff when the Unix socket listen queue is momentarily full
    }
    return -1;
}

bool writeAll(int fd, const char* data, std::size_t size)
{
    while (size > 0) {
        const ssize_t written = ::write(fd, data, std::min(size, kChunkBytes));
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return false;
        }
        data += written;
        size -= static_cast<std::size_t>(written);
    }
    return true;
}

// Reads what is there, up to a chunk; 0 at the end or on an error.
std::size_t readSome(int fd, char* data)
{
    for (;;) {
        const ssize_t got = ::read(fd, data, kChunkBytes);
        if (got < 0 && errno == EINTR) {
            continue;
        }
        return got > 0 ? static_cast<std::size_t>(got) : 0;
    }
}

long long expectedReplies(std::string_view line)
{
    const Json message = Json::parse(line, nullptr, false);
    if (!message.is_object() || !message.contains("method")) {
        return 0;
    }
    if (message.contains("id")) {
        return 1;
    }
    const Json& method = message["method"];
    return method.is_string() && method.get_ref<const std::string&>() == "notifications/cancelled" ? -1 : 0;
}

class Replies
{
public:
    long long count(const char* data, std::size_t size)
    {
        long long answered = 0;
        for (std::size_t index = 0; index < size; ++index) {
            const char c = data[index];
            if (c != '\n') {
                if (m_head.size() < kSniffBytes) {
                    m_head += c;
                } else {
                    m_long = true;
                }
                continue;
            }
            answered += (m_long || answers(m_head)) ? 1 : 0;
            m_head.clear();
            m_long = false;
        }
        return answered;
    }

private:
    static constexpr std::size_t kSniffBytes = 4096;

    static bool answers(std::string_view line)
    {
        const Json message = Json::parse(line, nullptr, false);
        return !message.is_object() || !message.contains("method");
    }

    std::string m_head;
    bool m_long = false;
};

void reportOff()
{
    std::cerr << "gs mcp: " << kOffText << '\n';
}

int refuse()
{
    const int input = STDIN_FILENO;
    const int output = STDOUT_FILENO;
    std::string buffer;
    std::vector<char> chunk(kChunkBytes);
    for (;;) {
        for (std::size_t newline = buffer.find('\n'); newline != std::string::npos; newline = buffer.find('\n')) {
            const std::string line = buffer.substr(0, newline);
            buffer.erase(0, newline + 1);
            if (!jsonNestingWithin(line, kMostJsonNesting)) {
                continue;
            }
            const Json message = Json::parse(line, nullptr, false);
            if (message.is_object() && message.contains("method") && message.contains("id")) {
                const Json reply = {{"jsonrpc", "2.0"},
                                    {"id", message.at("id")},
                                    {"error", {{"code", kRefused}, {"message", kOffText}}}};
                const std::string text = reply.dump(-1, ' ', false, Json::error_handler_t::replace) + "\n";
                static_cast<void>(writeAll(output, text.data(), text.size()));
                reportOff();
                return kProblems;
            }
        }
        const std::size_t read = buffer.size() > kMostLineBytes ? 0 : readSome(input, chunk.data());
        if (read == 0) {
            reportOff();
            return kProblems;
        }
        buffer.append(chunk.data(), read);
    }
}

void forward(int pipe, std::shared_ptr<Shared> shared)
{
    std::vector<char> chunk(kChunkBytes);
    std::string line;
    for (;;) {
        const std::size_t read = readSome(STDIN_FILENO, chunk.data());
        if (read == 0) {
            break;
        }
        long long expected = 0;
        for (std::size_t index = 0; index < read; ++index) {
            const char c = chunk[index];
            if (c != '\n') {
                if (line.size() <= kMostLineBytes) {
                    line += c;
                }
                continue;
            }
            expected += expectedReplies(line);
            line.clear();
        }
        {
            const std::scoped_lock lock(shared->mutex);
            shared->outstanding += expected;
        }
        if (!writeAll(pipe, chunk.data(), read)) {
            break;
        }
    }
    bool done = false;
    {
        const std::scoped_lock lock(shared->mutex);
        shared->inputEnded = true;
        done = shared->outstanding <= 0;
    }
    if (done) {
        // Wakes the read in relay(), as CancelIoEx does on Windows.
        ::shutdown(pipe, SHUT_RDWR);
    }
}

int relay(int pipe)
{
    auto shared = std::make_shared<Shared>();
    std::thread(forward, pipe, shared).detach();
    std::vector<char> chunk(kChunkBytes);
    Replies replies;
    for (;;) {
        const std::size_t read = readSome(pipe, chunk.data());
        if (read == 0) {
            break;
        }
        if (!writeAll(STDOUT_FILENO, chunk.data(), read)) {
            break;
        }
        const long long answered = replies.count(chunk.data(), read);
        const std::scoped_lock lock(shared->mutex);
        shared->outstanding -= answered;
        if (shared->inputEnded && shared->outstanding <= 0) {
            return kSuccess;
        }
    }
    const std::scoped_lock lock(shared->mutex);
    if (shared->inputEnded) {
        return kSuccess;
    }
    std::cerr << "gs mcp: anmacgamer closed the connection\n";
    return kProblems;
}

}

int runMcpRelay(const std::vector<std::wstring_view>& args)
{
    if (!args.empty()) {
        std::cerr << "usage: gs mcp\n";
        return kUsage;
    }
    const int pipe = connectTo(socketPath());
    if (pipe < 0) {
        return refuse();
    }
    return relay(pipe);
}

}
