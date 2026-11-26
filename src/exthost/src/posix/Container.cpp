// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Containers of restricted native extensions off Windows (see posix/Sandbox.h).
#include "../Container.h"
#include "Bridge.h"
#include "Sandbox.h"

#include "gs/exthost/Hosted.h"

#include <gs/platform/Crypto.h>

#include <sys/stat.h>

#include <cstdint>
#include <cstdlib>
#include <format>
#include <map>
#include <mutex>
#include <span>
#include <system_error>
#include <vector>

#if defined(__APPLE__)
extern "C" {
int sandbox_init(const char* profile, std::uint64_t flags, char** error);
void sandbox_free_error(char* error);
char* sandbox_extension_issue_file(const char* extensionClass, const char* path, std::uint32_t flags);
}
#endif

namespace gs::exthost {

namespace detail {

namespace {

constexpr std::size_t kNameBytes = 16;

struct Shared {
    std::mutex mutex;
    std::map<std::string, std::vector<std::string>, std::less<>> tokens;
};

Shared& shared()
{
    static Shared all;
    return all;
}

std::string narrow(const std::wstring& text)
{
    std::string out;
    for (const wchar_t c : text) {
        out.push_back(static_cast<char>(c));
    }
    return out;
}

std::filesystem::path dataFolder()
{
    const char* home = std::getenv("HOME");
#if defined(__APPLE__)
    if (home == nullptr || *home != '/') {
        return {};
    }
    return std::filesystem::path(home) / "Library" / "Application Support" / "anmacgamer" / "Containers";
#else
    if (const char* data = std::getenv("XDG_DATA_HOME"); data != nullptr && *data == '/') {
        return std::filesystem::path(data) / "GameScrcpy" / "Containers";
    }
    if (home == nullptr || *home != '/') {
        return {};
    }
    return std::filesystem::path(home) / ".local" / "share" / "GameScrcpy" / "Containers";
#endif
}

// A path as text inside an SBPL string: backslashes and quotes escaped.
std::string quotedPath(const std::filesystem::path& path)
{
    std::string out = "\"";
    for (const char c : path.string()) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out + "\"";
}

std::filesystem::path real(const char* text)
{
    if (text == nullptr || *text == '\0') {
        return {};
    }
    std::error_code error;
    std::filesystem::path path = std::filesystem::canonical(text, error);
    return error ? std::filesystem::path() : path;
}

}

std::wstring containerName(std::string_view extension)
{
    const platform::Sha256Digest digest =
        platform::sha256(std::span(reinterpret_cast<const std::uint8_t*>(extension.data()), extension.size()));
    std::wstring name = L"GameScrcpy.Extension.";
    for (std::size_t index = 0; index < kNameBytes; ++index) {
        name += std::format(L"{:02X}", digest[index]);
    }
    return name;
}

std::wstring systemFolder()
{
    return L"/usr/bin";
}

namespace posix {

std::filesystem::path containerFolder(std::string_view extension, std::string& problem)
{
    const std::filesystem::path base = dataFolder();
    if (base.empty()) {
        problem = "the folder for extension containers cannot be found (HOME is not set)";
        return {};
    }
    const std::filesystem::path folder = base / narrow(containerName(extension));
    std::error_code error;
    std::filesystem::create_directories(folder / "tmp", error);
    if (error) {
        problem = "the extension's container folder cannot be made: " + error.message();
        return {};
    }
    ::chmod(folder.c_str(), 0700);
    std::filesystem::path made = std::filesystem::canonical(folder, error);
    if (error) {
        problem = "the extension's container folder cannot be found: " + error.message();
        return {};
    }
    return made;
}

std::string sandboxProfile(const std::filesystem::path& readable, const std::filesystem::path& container,
                           const std::filesystem::path& program)
{
    std::string profile = "(version 1)\n(deny default)\n";
    // The system's libraries and the little a C++ program needs from the system to run.
    profile += "(allow file-read* file-map-executable (subpath \"/usr/lib\") (subpath \"/System\")"
               " (subpath \"/Library/Apple/System\"))\n";
    profile += "(allow file-read-metadata)\n";
    profile += "(allow file-read* file-write-data (literal \"/dev/null\") (literal \"/dev/zero\")"
               " (literal \"/dev/random\") (literal \"/dev/urandom\"))\n";
    profile += "(allow sysctl-read)\n(allow signal (target self))\n(allow process-info* (target self))\n";
    // Shared memory for the channel and media rings, by the names posix/Compat.cpp gives it.
    profile += "(allow ipc-posix-shm-read* ipc-posix-shm-write-create ipc-posix-shm-write-data"
               " ipc-posix-shm-write-unlink (ipc-posix-name-prefix \"/gsx.\"))\n";
    // gs-exthost itself and its folder, the extension's folder (read only), its container (read and write).
    if (!program.empty()) {
        profile += "(allow file-read* file-map-executable (subpath " + quotedPath(program.parent_path()) + "))\n";
    }
    if (!readable.empty()) {
        profile += "(allow file-read* file-map-executable (subpath " + quotedPath(readable) + "))\n";
    }
    if (!container.empty()) {
        profile += "(allow file-read* file-write* (subpath " + quotedPath(container) + "))\n";
    }
    // Files the app shares one by one (shareWithContainer).
    profile += "(allow file-read* (extension \"com.apple.app-sandbox.read\"))\n";
    return profile;
}

bool enterSandbox(std::string& problem)
{
#if defined(__APPLE__)
    const std::filesystem::path readable = real(std::getenv(kReadableVariable));
    const std::filesystem::path container = real(std::getenv(kContainerVariable));
    const std::filesystem::path program = real(std::getenv(kProgramVariable));
    if (readable.empty() || container.empty()) {
        problem = "the extension's folders were not given";
        return false;
    }
    char* error = nullptr;
    const bool entered = sandbox_init(sandboxProfile(readable, container, program).c_str(), 0, &error) == 0;
    if (error != nullptr) {
        problem = error;
        sandbox_free_error(error);
    }
    return entered;
#else
    problem = "restricted extensions need the macOS sandbox";
    return false;
#endif
}

std::vector<std::string> sharedTokens(const std::string& container)
{
    Shared& all = shared();
    const std::scoped_lock lock(all.mutex);
    const auto found = all.tokens.find(container);
    return found != all.tokens.end() ? found->second : std::vector<std::string>();
}

}

}

bool forgetContainer(std::string_view extension)
{
    const std::string name = detail::narrow(detail::containerName(extension));
    {
        detail::Shared& all = detail::shared();
        const std::scoped_lock lock(all.mutex);
        all.tokens.erase(name);
    }
    const std::filesystem::path base = detail::dataFolder();
    if (base.empty()) {
        return false;
    }
    std::error_code error;
    std::filesystem::remove_all(base / name, error);
    return !error;
}

bool shareWithContainer(std::string_view extension, const std::filesystem::path& file, std::string& problem)
{
#if defined(__APPLE__)
    std::error_code error;
    const std::filesystem::path path = std::filesystem::canonical(file, error);
    if (error) {
        problem = "the file to share cannot be found: " + error.message();
        return false;
    }
    char* issued = sandbox_extension_issue_file("com.apple.app-sandbox.read", path.c_str(), 0);
    if (issued == nullptr) {
        problem = "macOS would not share the file with the extension's sandbox";
        return false;
    }
    const std::string token(issued);
    std::free(issued);
    const std::string name = detail::narrow(detail::containerName(extension));
    {
        detail::Shared& all = detail::shared();
        const std::scoped_lock lock(all.mutex);
        all.tokens[name].push_back(token);
    }
    detail::posix::grantToContainer(name, token);
    return true;
#else
    static_cast<void>(extension);
    static_cast<void>(file);
    problem = "restricted extensions need the macOS sandbox";
    return false;
#endif
}

}
