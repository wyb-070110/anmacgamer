// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The Win32 calls of posix/compat/windows.h on POSIX (macOS, and Linux for the port's tests). See that header for
// what a HANDLE holds, and posix/Bridge.h for how handles cross between the app and gs-exthost.
#include "Bridge.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <crt_externs.h>
#include <sys/event.h>

extern "C" std::int64_t sandbox_extension_consume(const char* token);
#else
#include <sys/syscall.h>
#include <sys/timerfd.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

// --- what a HANDLE holds --------------------------------------------------------------------------------------

enum class Tag : std::uintptr_t { Object = 0, Event = 1, Mapping = 2, Watch = 3, Remote = 4, File = 5, Self = 6 };

constexpr unsigned kTagShift = 60;
constexpr std::uintptr_t kFdMask = (std::uintptr_t{1} << 24) - 1;
constexpr std::uintptr_t kManualBit = std::uintptr_t{1} << 48;
constexpr int kMostDescriptor = static_cast<int>(kFdMask);

std::uintptr_t raw(HANDLE handle) noexcept
{
    return reinterpret_cast<std::uintptr_t>(handle);
}

HANDLE handleOf(std::uintptr_t value) noexcept
{
    return reinterpret_cast<HANDLE>(value);
}

bool invalid(HANDLE handle) noexcept
{
    return handle == nullptr || handle == INVALID_HANDLE_VALUE;
}

Tag tagOf(HANDLE handle) noexcept
{
    return static_cast<Tag>(raw(handle) >> kTagShift);
}

int firstFd(HANDLE handle) noexcept
{
    return static_cast<int>(raw(handle) & kFdMask);
}

int secondFd(HANDLE handle) noexcept
{
    return static_cast<int>((raw(handle) >> 24) & kFdMask);
}

bool manual(HANDLE handle) noexcept
{
    return (raw(handle) & kManualBit) != 0;
}

HANDLE make(Tag tag, int first, int second = 0, bool manualReset = false) noexcept
{
    return handleOf((static_cast<std::uintptr_t>(tag) << kTagShift) | (static_cast<std::uintptr_t>(second) << 24) |
                    static_cast<std::uintptr_t>(first) | (manualReset ? kManualBit : 0));
}

const HANDLE kSelf = make(Tag::Self, 0);

thread_local DWORD t_lastError = ERROR_SUCCESS;

DWORD errorOf(int error) noexcept
{
    switch (error) {
    case ENOENT:
        return ERROR_FILE_NOT_FOUND;
    case ENOTDIR:
        return ERROR_PATH_NOT_FOUND;
    case EACCES:
    case EPERM:
        return ERROR_ACCESS_DENIED;
    case EBADF:
        return ERROR_INVALID_HANDLE;
    case ENOMEM:
        return ERROR_NOT_ENOUGH_MEMORY;
    case EEXIST:
        return ERROR_ALREADY_EXISTS;
    case ETIMEDOUT:
        return ERROR_TIMEOUT;
    default:
        return ERROR_INVALID_PARAMETER;
    }
}

BOOL fail(DWORD error) noexcept
{
    t_lastError = error;
    return FALSE;
}

BOOL failErrno() noexcept
{
    return fail(errorOf(errno));
}

bool setFlag(int fd, int getter, int setter, int flag, bool on) noexcept
{
    const int flags = fcntl(fd, getter);
    return flags >= 0 && fcntl(fd, setter, on ? (flags | flag) : (flags & ~flag)) == 0;
}

bool cloexec(int fd, bool on = true) noexcept
{
    return setFlag(fd, F_GETFD, F_SETFD, FD_CLOEXEC, on);
}

bool nonblocking(int fd) noexcept
{
    return setFlag(fd, F_GETFL, F_SETFL, O_NONBLOCK, true);
}

void closeFd(int fd) noexcept
{
    if (fd >= 0) {
        ::close(fd);
    }
}

std::string narrow(const wchar_t* text)
{
    std::string out;
    for (; text != nullptr && *text != 0; ++text) {
        const auto c = static_cast<std::uint32_t>(*text);
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return out;
}

std::wstring widen(std::string_view text)
{
    std::wstring out;
    for (std::size_t at = 0; at < text.size();) {
        const auto lead = static_cast<unsigned char>(text[at]);
        const int extra = lead < 0x80 ? 0 : (lead >> 5) == 0x6 ? 1 : (lead >> 4) == 0xE ? 2 : (lead >> 3) == 0x1E ? 3 : -1;
        if (extra < 0 || at + static_cast<std::size_t>(extra) >= text.size() + (extra == 0 ? 1 : 0)) {
            out.push_back(static_cast<wchar_t>(0xFFFD));
            ++at;
            continue;
        }
        std::uint32_t c = extra == 0 ? lead : lead & (0x3F >> extra);
        bool good = true;
        for (int k = 1; k <= extra; ++k) {
            const auto next = static_cast<unsigned char>(text[at + static_cast<std::size_t>(k)]);
            good = good && (next & 0xC0) == 0x80;
            c = (c << 6) | (next & 0x3F);
        }
        out.push_back(static_cast<wchar_t>(good ? c : 0xFFFD));
        at += good ? static_cast<std::size_t>(extra) + 1 : 1;
    }
    return out;
}

// --- objects only one process uses ----------------------------------------------------------------------------

struct Object {
    enum class Kind : std::uint8_t { Process, Timer, Job };
    explicit Object(Kind given) noexcept : kind(given) {}
    virtual ~Object() = default;
    Kind kind;
};

void leaveContainers(const void* process) noexcept;

struct Process final : Object {
    Process() noexcept : Object(Kind::Process) {}
    ~Process() override
    {
        leaveContainers(this);
        reap(false);
        closeFd(waitFd);
        closeFd(bridge);
    }

    // True once the process has ended; a child of ours is reaped then and its status kept.
    bool reap(bool block) noexcept
    {
        const std::scoped_lock lock(mutex);
        if (ended) {
            return true;
        }
        if (child) {
            int got = 0;
            const pid_t done = ::waitpid(pid, &got, block ? 0 : WNOHANG);
            if (done == pid) {
                status = got;
                ended = true;
            } else if (done < 0 && errno == ECHILD) {
                ended = true;
            }
        } else if (::kill(pid, 0) != 0 && errno == ESRCH) {
            ended = true;
        }
        return ended;
    }

    bool markEnded() noexcept
    {
        const std::scoped_lock lock(mutex);
        ended = true;
        return true;
    }

    pid_t pid = 0;
    bool child = false;
    int waitFd = -1;
    int bridge = -1;
    std::mutex mutex;
    std::mutex bridgeMutex;
    std::uint32_t nextToken = 1;
    bool ended = false;
    int status = 0;
    std::optional<DWORD> forced;
};

struct Timer final : Object {
    Timer() noexcept : Object(Kind::Timer) {}
    ~Timer() override { closeFd(fd); }
    int fd = -1;
};

struct Job final : Object {
    Job() noexcept : Object(Kind::Job) {}
};

Object* objectOf(HANDLE handle) noexcept
{
    return invalid(handle) || tagOf(handle) != Tag::Object ? nullptr : static_cast<Object*>(handle);
}

Process* processOf(HANDLE handle) noexcept
{
    Object* object = objectOf(handle);
    return object != nullptr && object->kind == Object::Kind::Process ? static_cast<Process*>(object) : nullptr;
}

Timer* timerOf(HANDLE handle) noexcept
{
    Object* object = objectOf(handle);
    return object != nullptr && object->kind == Object::Kind::Timer ? static_cast<Timer*>(object) : nullptr;
}

// A descriptor that turns readable when the process ends.
int watchProcess(pid_t pid, bool& ended) noexcept
{
#if defined(__APPLE__)
    const int queue = ::kqueue();
    if (queue < 0) {
        return -1;
    }
    cloexec(queue);
    struct kevent change{};
    EV_SET(&change, static_cast<uintptr_t>(pid), EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, nullptr);
    if (::kevent(queue, &change, 1, nullptr, 0, nullptr) != 0) {
        ended = errno == ESRCH;
    }
    return queue;
#else
    const int fd = static_cast<int>(::syscall(SYS_pidfd_open, pid, 0));
    if (fd < 0) {
        ended = errno == ESRCH;
        return -1;
    }
    cloexec(fd);
    return fd;
#endif
}

// --- the bridge -----------------------------------------------------------------------------------------------

// Grant: value is the length of a sandbox extension token that follows the note.
enum class Op : std::uint32_t { Give = 1, Close = 2, Ask = 3, Answer = 4, Grant = 5 };
constexpr std::size_t kMostTokenBytes = 4096;

struct Note {
    std::uint32_t op = 0;
    std::uint32_t token = 0;
    std::uint64_t value = 0;
};

constexpr std::size_t kMostPassed = 4;
// A write to a bridge whose other end is gone fails with EPIPE instead of raising SIGPIPE (macOS: SO_NOSIGPIPE on
// the socket, see Spawn.cpp).
#if defined(MSG_NOSIGNAL)
constexpr int kNoSignal = MSG_NOSIGNAL;
#else
constexpr int kNoSignal = 0;
#endif

bool sendNote(int socket, const Note& note, const std::vector<int>& fds) noexcept
{
    iovec part{const_cast<Note*>(&note), sizeof(note)};
    msghdr message{};
    message.msg_iov = &part;
    message.msg_iovlen = 1;
    alignas(cmsghdr) std::array<char, CMSG_SPACE(sizeof(int) * kMostPassed)> control{};
    if (!fds.empty()) {
        if (fds.size() > kMostPassed) {
            return false;
        }
        message.msg_control = control.data();
        message.msg_controllen = static_cast<socklen_t>(CMSG_SPACE(sizeof(int) * fds.size()));
        cmsghdr* header = CMSG_FIRSTHDR(&message);
        header->cmsg_level = SOL_SOCKET;
        header->cmsg_type = SCM_RIGHTS;
        header->cmsg_len = static_cast<socklen_t>(CMSG_LEN(sizeof(int) * fds.size()));
        std::memcpy(CMSG_DATA(header), fds.data(), sizeof(int) * fds.size());
    }
    for (;;) {
        const ssize_t sent = ::sendmsg(socket, &message, kNoSignal);
        if (sent == static_cast<ssize_t>(sizeof(note))) {
            return true;
        }
        if (sent < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
}

// Reads one note and the descriptors that came with it; false at the end of the stream or on an error.
bool receiveNote(int socket, Note& note, std::vector<int>& fds) noexcept
{
    fds.clear();
    iovec part{&note, sizeof(note)};
    msghdr message{};
    message.msg_iov = &part;
    message.msg_iovlen = 1;
    alignas(cmsghdr) std::array<char, CMSG_SPACE(sizeof(int) * kMostPassed)> control{};
    message.msg_control = control.data();
    message.msg_controllen = static_cast<socklen_t>(control.size());
    ssize_t got = 0;
    do {
        got = ::recvmsg(socket, &message, MSG_WAITALL);
    } while (got < 0 && errno == EINTR);
    for (cmsghdr* header = CMSG_FIRSTHDR(&message); header != nullptr; header = CMSG_NXTHDR(&message, header)) {
        if (header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_RIGHTS) {
            const std::size_t count = (header->cmsg_len - CMSG_LEN(0)) / sizeof(int);
            for (std::size_t index = 0; index < count; ++index) {
                int fd = -1;
                std::memcpy(&fd, CMSG_DATA(header) + index * sizeof(int), sizeof(int));
                cloexec(fd);
                fds.push_back(fd);
            }
        }
    }
    if (got != static_cast<ssize_t>(sizeof(note)) || (message.msg_flags & MSG_CTRUNC) != 0) {
        for (const int fd : fds) {
            closeFd(fd);
        }
        fds.clear();
        return false;
    }
    return true;
}

// A local handle like the one value names, made from the descriptors that arrived for it.
HANDLE rebuilt(std::uint64_t value, const std::vector<int>& fds) noexcept
{
    const HANDLE given = handleOf(static_cast<std::uintptr_t>(value));
    const Tag tag = tagOf(given);
    const bool good = std::ranges::all_of(fds, [](int fd) { return fd >= 0 && fd <= kMostDescriptor; });
    if (good && tag == Tag::Event && fds.size() == 2) {
        return make(Tag::Event, fds[0], fds[1], manual(given));
    }
    if (good && (tag == Tag::Mapping || tag == Tag::Watch || tag == Tag::File) && fds.size() == 1) {
        return make(tag, fds[0]);
    }
    for (const int fd : fds) {
        closeFd(fd);
    }
    return nullptr;
}

// The child's end: what the app sent, by number, and the thread that reads it.
class ChildBridge
{
public:
    static ChildBridge* get() noexcept
    {
        static ChildBridge* bridge = [] () -> ChildBridge* {
            const char* text = std::getenv(gs::exthost::detail::posix::kBridgeVariable);
            if (text == nullptr) {
                return nullptr;
            }
            const int fd = std::atoi(text);
            ::unsetenv(gs::exthost::detail::posix::kBridgeVariable);
            if (fd <= 2 || ::fcntl(fd, F_GETFD) < 0) {
                return nullptr;
            }
            cloexec(fd);
            return new ChildBridge(fd);
        }();
        return bridge;
    }

    HANDLE take(std::uint32_t token, bool remove) noexcept
    {
        std::unique_lock lock(m_mutex);
        const bool arrived = m_arrived.wait_for(lock, std::chrono::seconds(5), [this, token] {
            return m_given.contains(token) || m_closed;
        });
        const auto found = m_given.find(token);
        if (!arrived || found == m_given.end()) {
            return nullptr;
        }
        const HANDLE local = found->second;
        if (remove) {
            m_given.erase(found);
        }
        return local;
    }

private:
    explicit ChildBridge(int fd) : m_fd(fd)
    {
        std::thread([this] { run(); }).detach();
    }

    void run() noexcept
    {
        Note note;
        std::vector<int> fds;
        while (receiveNote(m_fd, note, fds)) {
            switch (static_cast<Op>(note.op)) {
            case Op::Give: {
                const HANDLE local = rebuilt(note.value, fds);
                const std::scoped_lock lock(m_mutex);
                if (local != nullptr) {
                    m_given[note.token] = local;
                }
                m_arrived.notify_all();
                break;
            }
            case Op::Close:
                if (const HANDLE local = takeNow(note.token)) {
                    CloseHandle(local);
                }
                break;
            case Op::Ask:
                answer(note);
                break;
            case Op::Grant:
                if (!consume(note.value)) {
                    ::_exit(3);
                }
                break;
            default:
                break;
            }
            for (const int fd : fds) {
                if (static_cast<Op>(note.op) != Op::Give) {
                    closeFd(fd);
                }
            }
        }
        // The app's end closed: the app is gone, or done with this process.
        ::_exit(3);
    }

    // Reads the token that follows a Grant note and consumes it; false if the stream broke.
    bool consume(std::uint64_t length) noexcept
    {
        if (length == 0 || length > kMostTokenBytes) {
            return false;
        }
        std::string token(static_cast<std::size_t>(length), '\0');
        std::size_t got = 0;
        while (got < token.size()) {
            const ssize_t read = ::read(m_fd, token.data() + got, token.size() - got);
            if (read <= 0 && !(read < 0 && errno == EINTR)) {
                return false;
            }
            got += read > 0 ? static_cast<std::size_t>(read) : 0;
        }
#if defined(__APPLE__)
        static_cast<void>(sandbox_extension_consume(token.c_str()));
#endif
        return true;
    }

    HANDLE takeNow(std::uint32_t token) noexcept
    {
        const std::scoped_lock lock(m_mutex);
        const auto found = m_given.find(token);
        if (found == m_given.end()) {
            return nullptr;
        }
        const HANDLE local = found->second;
        m_given.erase(found);
        return local;
    }

    void answer(const Note& asked) noexcept
    {
        HANDLE named = handleOf(static_cast<std::uintptr_t>(asked.value));
        if (tagOf(named) == Tag::Remote) {
            const std::scoped_lock lock(m_mutex);
            const auto found = m_given.find(static_cast<std::uint32_t>(raw(named) & 0xFFFFFFFFu));
            named = found != m_given.end() ? found->second : nullptr;
        }
        Note reply{static_cast<std::uint32_t>(Op::Answer), asked.token, static_cast<std::uint64_t>(raw(named))};
        std::vector<int> fds;
        if (!invalid(named)) {
            fds = gs::exthost::detail::posix::descriptorsOf(named);
        }
        sendNote(m_fd, reply, fds);
    }

    int m_fd;
    std::mutex m_mutex;
    std::condition_variable m_arrived;
    std::map<std::uint32_t, HANDLE> m_given;
    bool m_closed = false;
};

// The local handle a handle stands for: a remote one becomes what arrived for it.
HANDLE local(HANDLE handle, bool remove = false) noexcept
{
    if (invalid(handle) || tagOf(handle) != Tag::Remote) {
        return handle;
    }
    ChildBridge* bridge = ChildBridge::get();
    return bridge != nullptr ? bridge->take(static_cast<std::uint32_t>(raw(handle) & 0xFFFFFFFFu), remove) : nullptr;
}

void startBridge() noexcept
{
    static_cast<void>(ChildBridge::get());
}

// --- views ----------------------------------------------------------------------------------------------------

std::mutex g_viewsMutex;
std::map<const void*, std::size_t> g_views;

// --- events ---------------------------------------------------------------------------------------------------

// Takes what is waiting in an event's pipe; true if there was something (the event was set).
bool drain(int fd) noexcept
{
    bool any = false;
    std::array<char, 64> sink{};
    for (;;) {
        const ssize_t got = ::read(fd, sink.data(), sink.size());
        if (got > 0) {
            any = true;
            continue;
        }
        if (got < 0 && errno == EINTR) {
            continue;
        }
        return any;
    }
}

bool consumeTimer(Timer& timer) noexcept
{
#if defined(__APPLE__)
    struct kevent fired{};
    const timespec now{0, 0};
    return ::kevent(timer.fd, nullptr, 0, &fired, 1, &now) == 1;
#else
    std::uint64_t expirations = 0;
    return ::read(timer.fd, &expirations, sizeof(expirations)) == static_cast<ssize_t>(sizeof(expirations));
#endif
}

// One handle being waited on: the descriptor to poll and what being readable means for it.
struct Waited {
    HANDLE handle = nullptr;
    int fd = -1;
};

// Whether a handle whose descriptor turned readable is signalled; an auto-reset event or a timer is consumed.
bool claim(const Waited& waited) noexcept
{
    switch (tagOf(waited.handle)) {
    case Tag::Event:
        return manual(waited.handle) ? true : drain(firstFd(waited.handle));
    case Tag::Watch:
        return true;
    case Tag::Object:
        if (Process* process = processOf(waited.handle)) {
            // Its watch fired: a child is reaped; another process may linger a moment as a zombie, but it ended.
            return process->child ? process->reap(true) : process->markEnded();
        }
        if (Timer* timer = timerOf(waited.handle)) {
            return consumeTimer(*timer);
        }
        return false;
    default:
        return false;
    }
}

}

namespace {

std::mutex g_containersMutex;
std::multimap<std::string, Process*> g_containers;

void leaveContainers(const void* process) noexcept
{
    const std::scoped_lock lock(g_containersMutex);
    std::erase_if(g_containers, [process](const auto& entry) { return entry.second == process; });
}

bool sendGrant(Process& process, std::string_view token) noexcept
{
    if (process.bridge < 0 || token.empty() || token.size() > kMostTokenBytes) {
        return false;
    }
    const std::scoped_lock lock(process.bridgeMutex);
    if (!sendNote(process.bridge, {static_cast<std::uint32_t>(Op::Grant), 0, token.size()}, {})) {
        return false;
    }
    std::size_t sent = 0;
    while (sent < token.size()) {
        const ssize_t wrote = ::send(process.bridge, token.data() + sent, token.size() - sent, kNoSignal);
        if (wrote <= 0 && !(wrote < 0 && errno == EINTR)) {
            return false;
        }
        sent += wrote > 0 ? static_cast<std::size_t>(wrote) : 0;
    }
    return true;
}

}

namespace gs::exthost::detail::posix {

void joinContainer(HANDLE process, const std::string& container)
{
    if (Process* found = processOf(process)) {
        const std::scoped_lock lock(g_containersMutex);
        g_containers.emplace(container, found);
    }
}

void grantToContainer(const std::string& container, std::string_view token)
{
    const std::scoped_lock lock(g_containersMutex);
    const auto [first, last] = g_containers.equal_range(container);
    for (auto entry = first; entry != last; ++entry) {
        sendGrant(*entry->second, token);
    }
}

bool grantToProcess(HANDLE process, std::string_view token)
{
    Process* found = processOf(process);
    return found != nullptr && sendGrant(*found, token);
}

std::vector<int> descriptorsOf(HANDLE handle)
{
    handle = local(handle);
    if (invalid(handle)) {
        return {};
    }
    switch (tagOf(handle)) {
    case Tag::Event:
        return {firstFd(handle), secondFd(handle)};
    case Tag::Mapping:
    case Tag::Watch:
    case Tag::File:
        return {firstFd(handle)};
    default:
        return {};
    }
}

HANDLE adoptChild(pid_t pid, int bridge) noexcept
{
    auto* process = new (std::nothrow) Process();
    if (process == nullptr) {
        closeFd(bridge);
        return nullptr;
    }
    process->pid = pid;
    process->child = true;
    process->bridge = bridge;
    bool ended = false;
    process->waitFd = watchProcess(pid, ended);
    if (ended) {
        process->reap(true);
    }
    return process;
}

pid_t processIdOf(HANDLE process) noexcept
{
    const Process* found = processOf(process);
    return found != nullptr ? found->pid : 0;
}

bool closeOnExec(int descriptor, bool on) noexcept
{
    return cloexec(descriptor, on);
}

}

DWORD GetLastError() noexcept
{
    return t_lastError;
}

void SetLastError(DWORD error) noexcept
{
    t_lastError = error;
}

BOOL CloseHandle(HANDLE handle) noexcept
{
    if (invalid(handle)) {
        return fail(ERROR_INVALID_HANDLE);
    }
    if (tagOf(handle) == Tag::Remote) {
        handle = local(handle, true);
        if (invalid(handle)) {
            return fail(ERROR_INVALID_HANDLE);
        }
    }
    switch (tagOf(handle)) {
    case Tag::Event:
        closeFd(firstFd(handle));
        closeFd(secondFd(handle));
        return TRUE;
    case Tag::Mapping:
    case Tag::Watch:
    case Tag::File:
        closeFd(firstFd(handle));
        return TRUE;
    case Tag::Self:
        return TRUE;
    case Tag::Object:
        delete static_cast<Object*>(handle);
        return TRUE;
    default:
        return fail(ERROR_INVALID_HANDLE);
    }
}

HANDLE CreateEventW(void*, BOOL manualReset, BOOL initialState, const wchar_t* name) noexcept
{
    startBridge();
    if (name != nullptr) {
        fail(ERROR_NOT_SUPPORTED);
        return nullptr;
    }
    std::array<int, 2> ends{-1, -1};
    if (::pipe(ends.data()) != 0) {
        failErrno();
        return nullptr;
    }
    if (ends[0] > kMostDescriptor || ends[1] > kMostDescriptor || !cloexec(ends[0]) || !cloexec(ends[1]) ||
        !nonblocking(ends[0]) || !nonblocking(ends[1])) {
        closeFd(ends[0]);
        closeFd(ends[1]);
        fail(ERROR_NOT_ENOUGH_MEMORY);
        return nullptr;
    }
    const HANDLE event = make(Tag::Event, ends[0], ends[1], manualReset != FALSE);
    if (initialState != FALSE) {
        SetEvent(event);
    }
    return event;
}

BOOL SetEvent(HANDLE event) noexcept
{
    event = local(event);
    if (invalid(event) || tagOf(event) != Tag::Event) {
        return fail(ERROR_INVALID_HANDLE);
    }
    const char one = 1;
    for (;;) {
        const ssize_t wrote = ::write(secondFd(event), &one, 1);
        // A full pipe already says "set".
        if (wrote == 1 || (wrote < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
            return TRUE;
        }
        if (wrote < 0 && errno == EINTR) {
            continue;
        }
        return failErrno();
    }
}

BOOL ResetEvent(HANDLE event) noexcept
{
    event = local(event);
    if (invalid(event) || tagOf(event) != Tag::Event) {
        return fail(ERROR_INVALID_HANDLE);
    }
    drain(firstFd(event));
    return TRUE;
}

DWORD WaitForMultipleObjects(DWORD count, const HANDLE* handles, BOOL waitAll, DWORD milliseconds) noexcept
{
    startBridge();
    if (count == 0 || count > MAXIMUM_WAIT_OBJECTS || handles == nullptr || waitAll != FALSE) {
        fail(ERROR_INVALID_PARAMETER);
        return WAIT_FAILED;
    }
    std::vector<Waited> waits(count);
    std::vector<pollfd> polled(count);
    for (DWORD index = 0; index < count; ++index) {
        Waited& waited = waits[index];
        waited.handle = local(handles[index]);
        if (invalid(waited.handle)) {
            fail(ERROR_INVALID_HANDLE);
            return WAIT_FAILED;
        }
        switch (tagOf(waited.handle)) {
        case Tag::Event:
        case Tag::Watch:
            waited.fd = firstFd(waited.handle);
            break;
        case Tag::Object:
            if (Process* process = processOf(waited.handle)) {
                waited.fd = process->waitFd;
            } else if (Timer* timer = timerOf(waited.handle)) {
                waited.fd = timer->fd;
            }
            break;
        default:
            break;
        }
        if (waited.fd < 0 && processOf(waited.handle) == nullptr) {
            fail(ERROR_INVALID_HANDLE);
            return WAIT_FAILED;
        }
        polled[index] = pollfd{waited.fd, POLLIN, 0};
    }
    const bool forever = milliseconds == INFINITE;
    const Clock::time_point until = Clock::now() + std::chrono::milliseconds(forever ? 0 : milliseconds);
    for (;;) {
        // An ended process stays signalled; check it before sleeping on descriptors.
        for (DWORD index = 0; index < count; ++index) {
            if (Process* process = processOf(waits[index].handle); process != nullptr && process->reap(false)) {
                return WAIT_OBJECT_0 + index;
            }
        }
        int timeout = -1;
        if (!forever) {
            const auto left = std::chrono::ceil<std::chrono::milliseconds>(until - Clock::now()).count();
            timeout = static_cast<int>(std::max<std::int64_t>(left, 0));
        }
        // A child whose watch could not be made is looked at again every 50 ms.
        if (std::ranges::any_of(polled, [](const pollfd& p) { return p.fd < 0; })) {
            timeout = timeout < 0 ? 50 : std::min(timeout, 50);
        }
        for (pollfd& p : polled) {
            p.revents = 0;
        }
        const int ready = ::poll(polled.data(), static_cast<nfds_t>(count), timeout);
        if (ready < 0 && errno != EINTR) {
            failErrno();
            return WAIT_FAILED;
        }
        for (DWORD index = 0; ready > 0 && index < count; ++index) {
            if ((polled[index].revents & (POLLIN | POLLHUP | POLLERR)) != 0 && claim(waits[index])) {
                return WAIT_OBJECT_0 + index;
            }
        }
        if (!forever && Clock::now() >= until) {
            return WAIT_TIMEOUT;
        }
    }
}

DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds) noexcept
{
    return WaitForMultipleObjects(1, &handle, FALSE, milliseconds);
}

HANDLE CreateFileMappingW(HANDLE file, void*, DWORD, DWORD sizeHigh, DWORD sizeLow, const wchar_t* name) noexcept
{
    startBridge();
    if (file != INVALID_HANDLE_VALUE || name != nullptr) {
        fail(ERROR_NOT_SUPPORTED);
        return nullptr;
    }
    const auto size = static_cast<off_t>((static_cast<std::uint64_t>(sizeHigh) << 32) | sizeLow);
    int fd = -1;
#if defined(__APPLE__)
    static std::atomic<std::uint32_t> counter{0};
    for (int attempt = 0; attempt < 8 && fd < 0; ++attempt) {
        char shmName[32];
        std::snprintf(shmName, sizeof(shmName), "/gsx.%d.%u", static_cast<int>(::getpid()), counter.fetch_add(1));
        fd = ::shm_open(shmName, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            ::shm_unlink(shmName);
        } else if (errno != EEXIST) {
            break;
        }
    }
#else
    fd = static_cast<int>(::syscall(SYS_memfd_create, "gs-exthost", 1U /* MFD_CLOEXEC */));
#endif
    if (fd < 0) {
        failErrno();
        return nullptr;
    }
    if (fd > kMostDescriptor || !cloexec(fd) || ::ftruncate(fd, size) != 0) {
        failErrno();
        closeFd(fd);
        return nullptr;
    }
    return make(Tag::Mapping, fd);
}

LPVOID MapViewOfFile(HANDLE mapping, DWORD access, DWORD offsetHigh, DWORD offsetLow, SIZE_T bytes) noexcept
{
    mapping = local(mapping);
    if (invalid(mapping) || tagOf(mapping) != Tag::Mapping) {
        fail(ERROR_INVALID_HANDLE);
        return nullptr;
    }
    const int fd = firstFd(mapping);
    const auto offset = static_cast<off_t>((static_cast<std::uint64_t>(offsetHigh) << 32) | offsetLow);
    if (bytes == 0) {
        struct stat status{};
        if (::fstat(fd, &status) != 0 || status.st_size <= offset) {
            failErrno();
            return nullptr;
        }
        bytes = static_cast<SIZE_T>(status.st_size - offset);
    }
    const bool writable = (access & FILE_MAP_WRITE) != 0 || access == FILE_MAP_ALL_ACCESS;
    void* view = ::mmap(nullptr, bytes, PROT_READ | (writable ? PROT_WRITE : 0), MAP_SHARED, fd, offset);
    if (view == MAP_FAILED) {
        failErrno();
        return nullptr;
    }
    const std::scoped_lock lock(g_viewsMutex);
    g_views[view] = bytes;
    return view;
}

BOOL UnmapViewOfFile(const void* view) noexcept
{
    std::size_t bytes = 0;
    {
        const std::scoped_lock lock(g_viewsMutex);
        const auto found = g_views.find(view);
        if (found == g_views.end()) {
            return fail(ERROR_INVALID_PARAMETER);
        }
        bytes = found->second;
        g_views.erase(found);
    }
    return ::munmap(const_cast<void*>(view), bytes) == 0 ? TRUE : failErrno();
}

HANDLE GetCurrentProcess() noexcept
{
    return kSelf;
}

DWORD GetCurrentProcessId() noexcept
{
    return static_cast<DWORD>(::getpid());
}

DWORD GetCurrentThreadId() noexcept
{
#if defined(__APPLE__)
    std::uint64_t id = 0;
    pthread_threadid_np(nullptr, &id);
    return static_cast<DWORD>(id);
#else
    return static_cast<DWORD>(::syscall(SYS_gettid));
#endif
}

HANDLE OpenProcess(DWORD, BOOL, DWORD processId) noexcept
{
    startBridge();
    if (processId == 0) {
        fail(ERROR_INVALID_PARAMETER);
        return nullptr;
    }
    auto* process = new (std::nothrow) Process();
    if (process == nullptr) {
        fail(ERROR_NOT_ENOUGH_MEMORY);
        return nullptr;
    }
    process->pid = static_cast<pid_t>(processId);
    bool ended = false;
    process->waitFd = watchProcess(process->pid, ended);
    if (ended || (::kill(process->pid, 0) != 0 && errno == ESRCH)) {
        delete process;
        fail(ERROR_INVALID_PARAMETER);
        return nullptr;
    }
    return process;
}

BOOL TerminateProcess(HANDLE handle, UINT exitCode) noexcept
{
    if (handle == kSelf) {
        ::_exit(static_cast<int>(exitCode));
    }
    Process* process = processOf(handle);
    if (process == nullptr) {
        return fail(ERROR_INVALID_HANDLE);
    }
    {
        const std::scoped_lock lock(process->mutex);
        if (process->ended) {
            return fail(ERROR_ACCESS_DENIED);
        }
        if (!process->forced) {
            process->forced = exitCode;
        }
    }
    return ::kill(process->pid, SIGKILL) == 0 || errno == ESRCH ? TRUE : failErrno();
}

BOOL GetExitCodeProcess(HANDLE handle, DWORD* exitCode) noexcept
{
    Process* process = processOf(handle);
    if (process == nullptr || exitCode == nullptr) {
        return fail(ERROR_INVALID_HANDLE);
    }
    if (!process->reap(false)) {
        *exitCode = STILL_ACTIVE;
        return TRUE;
    }
    const std::scoped_lock lock(process->mutex);
    if (process->forced) {
        *exitCode = *process->forced;
    } else if (WIFEXITED(process->status)) {
        *exitCode = static_cast<DWORD>(WEXITSTATUS(process->status));
    } else if (WIFSIGNALED(process->status)) {
        // As the Windows exception code of the same fault, so the app reads it the same way.
        switch (WTERMSIG(process->status)) {
        case SIGSEGV:
        case SIGBUS:
            *exitCode = EXCEPTION_ACCESS_VIOLATION;
            break;
        case SIGILL:
            *exitCode = 0xC000001Du;
            break;
        case SIGFPE:
            *exitCode = 0xC0000094u;
            break;
        case SIGABRT:
            *exitCode = 0xC0000409u;
            break;
        default:
            *exitCode = 0xC0000000u | static_cast<DWORD>(WTERMSIG(process->status));
            break;
        }
    } else {
        *exitCode = 0;
    }
    return TRUE;
}

BOOL DuplicateHandle(HANDLE sourceProcess, HANDLE source, HANDLE targetProcess, HANDLE* target, DWORD, BOOL inherit,
                     DWORD options) noexcept
{
    startBridge();
    const bool closeSource = (options & DUPLICATE_CLOSE_SOURCE) != 0;
    // Within this process: new descriptors for the same thing.
    if (sourceProcess == kSelf && targetProcess == kSelf) {
        const HANDLE from = local(source);
        const std::vector<int> fds = gs::exthost::detail::posix::descriptorsOf(from);
        if (fds.empty() || target == nullptr) {
            return fail(ERROR_INVALID_HANDLE);
        }
        std::vector<int> copies;
        for (const int fd : fds) {
            const int copy = ::fcntl(fd, inherit != FALSE ? F_DUPFD : F_DUPFD_CLOEXEC, 3);
            if (copy < 0 || copy > kMostDescriptor) {
                closeFd(copy);
                for (const int made : copies) {
                    closeFd(made);
                }
                return failErrno();
            }
            copies.push_back(copy);
        }
        *target = rebuilt(static_cast<std::uint64_t>(raw(from)), copies);
        if (closeSource) {
            CloseHandle(source);
        }
        return *target != nullptr ? TRUE : fail(ERROR_INVALID_HANDLE);
    }
    // To a child: send the descriptors; the child's handle is their number until it uses it.
    if (sourceProcess == kSelf && target != nullptr) {
        Process* process = processOf(targetProcess);
        const HANDLE from = local(source);
        const std::vector<int> fds = gs::exthost::detail::posix::descriptorsOf(from);
        if (process == nullptr || process->bridge < 0 || fds.empty()) {
            return fail(ERROR_INVALID_HANDLE);
        }
        const std::scoped_lock lock(process->bridgeMutex);
        const std::uint32_t token = process->nextToken++;
        if (!sendNote(process->bridge, {static_cast<std::uint32_t>(Op::Give), token, static_cast<std::uint64_t>(raw(from))},
                      fds)) {
            return fail(ERROR_INVALID_HANDLE);
        }
        *target = handleOf((static_cast<std::uintptr_t>(Tag::Remote) << kTagShift) | token);
        if (closeSource) {
            CloseHandle(source);
        }
        return TRUE;
    }
    // Closing a handle in a child.
    if (target == nullptr && closeSource) {
        Process* process = processOf(sourceProcess);
        if (process == nullptr || process->bridge < 0 || tagOf(source) != Tag::Remote) {
            return fail(ERROR_INVALID_HANDLE);
        }
        const std::scoped_lock lock(process->bridgeMutex);
        return sendNote(process->bridge,
                        {static_cast<std::uint32_t>(Op::Close), static_cast<std::uint32_t>(raw(source) & 0xFFFFFFFFu), 0},
                        {})
                   ? TRUE
                   : fail(ERROR_INVALID_HANDLE);
    }
    // From a child: ask it for the descriptors behind the value it named.
    if (targetProcess == kSelf && target != nullptr) {
        Process* process = processOf(sourceProcess);
        if (process == nullptr || process->bridge < 0) {
            return fail(ERROR_INVALID_HANDLE);
        }
        const std::scoped_lock lock(process->bridgeMutex);
        const std::uint32_t token = process->nextToken++;
        if (!sendNote(process->bridge, {static_cast<std::uint32_t>(Op::Ask), token, static_cast<std::uint64_t>(raw(source))},
                      {})) {
            return fail(ERROR_INVALID_HANDLE);
        }
        const Clock::time_point until = Clock::now() + std::chrono::seconds(5);
        for (;;) {
            pollfd ready{process->bridge, POLLIN, 0};
            const auto left = std::chrono::ceil<std::chrono::milliseconds>(until - Clock::now()).count();
            if (left <= 0 || ::poll(&ready, 1, static_cast<int>(left)) <= 0) {
                return fail(ERROR_TIMEOUT);
            }
            Note note;
            std::vector<int> fds;
            if (!receiveNote(process->bridge, note, fds)) {
                return fail(ERROR_INVALID_HANDLE);
            }
            if (static_cast<Op>(note.op) != Op::Answer || note.token != token) {
                for (const int fd : fds) {
                    closeFd(fd);
                }
                continue;
            }
            *target = fds.empty() ? nullptr : rebuilt(note.value, fds);
            if (*target != nullptr && inherit == FALSE) {
                for (const int fd : gs::exthost::detail::posix::descriptorsOf(*target)) {
                    cloexec(fd);
                }
            }
            return *target != nullptr ? TRUE : fail(ERROR_INVALID_HANDLE);
        }
    }
    return fail(ERROR_NOT_SUPPORTED);
}

HANDLE CreateWaitableTimerExW(void*, const wchar_t* name, DWORD, DWORD) noexcept
{
    if (name != nullptr) {
        fail(ERROR_NOT_SUPPORTED);
        return nullptr;
    }
    auto timer = std::make_unique<Timer>();
#if defined(__APPLE__)
    timer->fd = ::kqueue();
#else
    timer->fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
#endif
    if (timer->fd < 0) {
        failErrno();
        return nullptr;
    }
    cloexec(timer->fd);
    return timer.release();
}

BOOL SetWaitableTimer(HANDLE handle, const LARGE_INTEGER* due, LONG period, void*, void*, BOOL) noexcept
{
    Timer* timer = timerOf(handle);
    if (timer == nullptr || due == nullptr || period != 0) {
        return fail(ERROR_INVALID_PARAMETER);
    }
    // Negative: relative, in 100 ns steps. Positive (an absolute time) is taken as now.
    const std::int64_t nanoseconds = due->QuadPart < 0 ? -due->QuadPart * 100 : 0;
    consumeTimer(*timer);
#if defined(__APPLE__)
    struct kevent change{};
    EV_SET(&change, 1, EVFILT_TIMER, EV_ADD | EV_ONESHOT, NOTE_NSECONDS, std::max<std::int64_t>(nanoseconds, 1),
           nullptr);
    return ::kevent(timer->fd, &change, 1, nullptr, 0, nullptr) == 0 ? TRUE : failErrno();
#else
    itimerspec when{};
    const std::int64_t at = std::max<std::int64_t>(nanoseconds, 1);
    when.it_value.tv_sec = static_cast<time_t>(at / 1'000'000'000);
    when.it_value.tv_nsec = static_cast<long>(at % 1'000'000'000);
    return ::timerfd_settime(timer->fd, 0, &when, nullptr) == 0 ? TRUE : failErrno();
#endif
}

BOOL CancelWaitableTimer(HANDLE handle) noexcept
{
    Timer* timer = timerOf(handle);
    if (timer == nullptr) {
        return fail(ERROR_INVALID_HANDLE);
    }
#if defined(__APPLE__)
    struct kevent change{};
    EV_SET(&change, 1, EVFILT_TIMER, EV_DELETE, 0, 0, nullptr);
    ::kevent(timer->fd, &change, 1, nullptr, 0, nullptr);
#else
    const itimerspec off{};
    ::timerfd_settime(timer->fd, 0, &off, nullptr);
#endif
    consumeTimer(*timer);
    return TRUE;
}

BOOL QueryPerformanceCounter(LARGE_INTEGER* counter) noexcept
{
    if (counter == nullptr) {
        return fail(ERROR_INVALID_PARAMETER);
    }
    counter->QuadPart = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
    return TRUE;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* frequency) noexcept
{
    if (frequency == nullptr) {
        return fail(ERROR_INVALID_PARAMETER);
    }
    frequency->QuadPart = 1'000'000'000;
    return TRUE;
}

void Sleep(DWORD milliseconds) noexcept // source-rule-allow R9: POSIX implementation of the Win32 Sleep compatibility shim
{
    if (milliseconds == 0) {
        std::this_thread::yield();
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds)); // source-rule-allow R9: POSIX implementation of the Win32 Sleep compatibility shim
    }
}

UINT SetErrorMode(UINT) noexcept
{
    return 0;
}

HMODULE LoadLibraryExW(const wchar_t* path, HANDLE file, DWORD flags) noexcept
{
    return LoadLibraryExW(narrow(path).c_str(), file, flags);
}

HMODULE LoadLibraryExW(const char* path, HANDLE, DWORD) noexcept
{
    const std::string file = path != nullptr ? path : "";
    void* library = ::dlopen(file.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        fail(::access(file.c_str(), R_OK) == 0 ? ERROR_MOD_NOT_FOUND : errorOf(errno));
    }
    return library;
}

FARPROC GetProcAddress(HMODULE library, const char* name) noexcept
{
    void* found = library != nullptr && name != nullptr ? ::dlsym(library, name) : nullptr;
    if (found == nullptr) {
        fail(ERROR_PROC_NOT_FOUND);
    }
    return reinterpret_cast<FARPROC>(found);
}

BOOL FreeLibrary(HMODULE library) noexcept
{
    return library != nullptr && ::dlclose(library) == 0 ? TRUE : fail(ERROR_INVALID_HANDLE);
}

DWORD GetEnvironmentVariableA(const char* name, char* buffer, DWORD size) noexcept
{
    const char* value = name != nullptr ? std::getenv(name) : nullptr;
    if (value == nullptr) {
        fail(204 /* ERROR_ENVVAR_NOT_FOUND */);
        return 0;
    }
    const std::size_t length = std::strlen(value);
    if (buffer == nullptr || length + 1 > size) {
        return static_cast<DWORD>(length + 1);
    }
    std::memcpy(buffer, value, length + 1);
    return static_cast<DWORD>(length);
}

DWORD GetEnvironmentVariableW(const wchar_t* name, wchar_t* buffer, DWORD size) noexcept
{
    const char* value = name != nullptr ? std::getenv(narrow(name).c_str()) : nullptr;
    if (value == nullptr) {
        fail(204 /* ERROR_ENVVAR_NOT_FOUND */);
        return 0;
    }
    const std::wstring wide = widen(value);
    if (buffer == nullptr || wide.size() + 1 > size) {
        return static_cast<DWORD>(wide.size() + 1);
    }
    std::wmemcpy(buffer, wide.c_str(), wide.size() + 1);
    return static_cast<DWORD>(wide.size());
}

const wchar_t* GetCommandLineW() noexcept
{
    static const std::wstring line = [] {
        std::wstring text;
#if defined(__APPLE__)
        const int count = *_NSGetArgc();
        char** words = *_NSGetArgv();
        for (int index = 0; index < count; ++index) {
            if (!text.empty()) {
                text.push_back(L' ');
            }
            text += widen(words[index]);
        }
#else
        if (std::FILE* file = std::fopen("/proc/self/cmdline", "rb")) {
            std::string all;
            for (int c = std::fgetc(file); c != EOF; c = std::fgetc(file)) {
                all.push_back(c == 0 ? ' ' : static_cast<char>(c));
            }
            std::fclose(file);
            text = widen(all);
        }
#endif
        return text;
    }();
    return line.c_str();
}

int MultiByteToWideChar(UINT, DWORD, const char* text, int bytes, wchar_t* wide, int size) noexcept
{
    if (text == nullptr) {
        return 0;
    }
    const std::size_t length = bytes < 0 ? std::strlen(text) + 1 : static_cast<std::size_t>(bytes);
    const std::wstring converted = widen(std::string_view(text, length));
    if (wide == nullptr || size == 0) {
        return static_cast<int>(converted.size());
    }
    if (converted.size() > static_cast<std::size_t>(size)) {
        fail(122 /* ERROR_INSUFFICIENT_BUFFER */);
        return 0;
    }
    std::wmemcpy(wide, converted.data(), converted.size());
    return static_cast<int>(converted.size());
}

HANDLE CreateFileW(const wchar_t* path, DWORD access, DWORD, void*, DWORD disposition, DWORD, HANDLE) noexcept
{
    int flags = O_CLOEXEC;
    const bool reading = (access & GENERIC_READ) != 0;
    const bool writing = (access & GENERIC_WRITE) != 0;
    flags |= reading && writing ? O_RDWR : writing ? O_WRONLY : O_RDONLY;
    switch (disposition) {
    case CREATE_NEW:
        flags |= O_CREAT | O_EXCL;
        break;
    case CREATE_ALWAYS:
        flags |= O_CREAT | O_TRUNC;
        break;
    case OPEN_ALWAYS:
        flags |= O_CREAT;
        break;
    default:
        break;
    }
    const int fd = ::open(narrow(path).c_str(), flags, 0600);
    if (fd < 0) {
        failErrno();
        return INVALID_HANDLE_VALUE;
    }
    if (fd > kMostDescriptor) {
        closeFd(fd);
        fail(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }
    return make(Tag::File, fd);
}

BOOL ReadFile(HANDLE file, void* buffer, DWORD wanted, DWORD* read, void*) noexcept
{
    file = local(file);
    if (invalid(file) || (tagOf(file) != Tag::File && tagOf(file) != Tag::Watch)) {
        return fail(ERROR_INVALID_HANDLE);
    }
    ssize_t got = 0;
    do {
        got = ::read(firstFd(file), buffer, wanted);
    } while (got < 0 && errno == EINTR);
    if (got < 0) {
        return failErrno();
    }
    if (read != nullptr) {
        *read = static_cast<DWORD>(got);
    }
    return TRUE;
}

BOOL WriteFile(HANDLE file, const void* buffer, DWORD size, DWORD* written, void*) noexcept
{
    file = local(file);
    if (invalid(file) || tagOf(file) != Tag::File) {
        return fail(ERROR_INVALID_HANDLE);
    }
    ssize_t wrote = 0;
    do {
        wrote = ::write(firstFd(file), buffer, size);
    } while (wrote < 0 && errno == EINTR);
    if (wrote < 0) {
        return failErrno();
    }
    if (written != nullptr) {
        *written = static_cast<DWORD>(wrote);
    }
    return TRUE;
}

void RaiseException(DWORD, DWORD, DWORD, const ULONG_PTR*) noexcept
{
    // Like an access violation: a write through a null pointer.
    volatile int* nothing = nullptr;
    *nothing = 0;
    __builtin_trap();
}
