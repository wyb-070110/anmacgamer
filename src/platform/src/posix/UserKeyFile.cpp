// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The per-user key in a file of the user's own (mode 0600) under $XDG_DATA_HOME/GameScrcpy. On macOS only
// userKeyInFile is used here, for programs outside an app bundle (see UserKeyKeychain.cpp).
#include "UserKey.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>

namespace gs::platform::detail {

namespace {

#if !defined(__APPLE__)
std::filesystem::path keyFile()
{
    if (const char* data = std::getenv("XDG_DATA_HOME"); data != nullptr && *data == '/') {
        return std::filesystem::path(data) / "GameScrcpy" / "user-protection.key";
    }
    const char* home = std::getenv("HOME");
    if (home == nullptr || *home != '/') {
        return {};
    }
    return std::filesystem::path(home) / ".local" / "share" / "GameScrcpy" / "user-protection.key";
}
#endif

bool readAll(int file, UserKey& key) noexcept
{
    std::size_t done = 0;
    while (done < key.size()) {
        const ssize_t read = ::read(file, key.data() + done, key.size() - done);
        if (read < 0 && errno == EINTR) {
            continue;
        }
        if (read <= 0) {
            return false;
        }
        done += static_cast<std::size_t>(read);
    }
    return true;
}

bool writeAll(int file, const UserKey& key) noexcept
{
    std::size_t done = 0;
    while (done < key.size()) {
        const ssize_t written = ::write(file, key.data() + done, key.size() - done);
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return false;
        }
        done += static_cast<std::size_t>(written);
    }
    return ::fsync(file) == 0;
}

std::optional<UserKey> load(const std::filesystem::path& path) noexcept
{
    const int file = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (file < 0) {
        return std::nullopt;
    }
    UserKey key{};
    struct stat status{};
    const bool ok = ::fstat(file, &status) == 0 && status.st_uid == ::getuid() && (status.st_mode & 077) == 0 &&
                    status.st_size == static_cast<off_t>(key.size()) && readAll(file, key);
    ::close(file);
    return ok ? std::optional<UserKey>(key) : std::nullopt;
}

std::optional<UserKey> create(const std::filesystem::path& path) noexcept
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return std::nullopt;
    }
    ::chmod(path.parent_path().c_str(), 0700);
    const std::filesystem::path temporary = path.string() + ".new";
    const int file = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (file < 0) {
        return std::nullopt;
    }
    UserKey key{};
    randomBytes(key);
    const bool written = writeAll(file, key);
    ::close(file);
    if (!written || ::link(temporary.c_str(), path.c_str()) != 0) {
        ::unlink(temporary.c_str());
        return load(path);
    }
    ::unlink(temporary.c_str());
    return key;
}

}

std::optional<UserKey> userKeyInFile(const std::filesystem::path& path) noexcept
{
    static std::mutex mutex;
    try {
        const std::lock_guard lock(mutex);
        if (path.empty()) {
            return std::nullopt;
        }
        if (std::optional<UserKey> existing = load(path)) {
            return existing;
        }
        return create(path);
    } catch (...) {
        return std::nullopt;
    }
}

#if !defined(__APPLE__)
std::optional<UserKey> userKey() noexcept
{
    try {
        return userKeyInFile(keyFile());
    } catch (...) {
        return std::nullopt;
    }
}
#endif

}
