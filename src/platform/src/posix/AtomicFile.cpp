// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// POSIX atomic files: write a sibling temporary, flush it to the disk (F_FULLFSYNC on macOS), rename it over
// the target and flush the directory. POSIX has no sharing modes, so a file is never busy here.
#include "gs/platform/AtomicFile.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <new>

namespace gs::platform {

namespace {

std::error_code lastError() noexcept
{
    return {errno, std::system_category()};
}

class Descriptor
{
public:
    explicit Descriptor(int fd) noexcept : m_fd(fd) {}
    ~Descriptor() { close(); }
    Descriptor(const Descriptor&) = delete;
    Descriptor& operator=(const Descriptor&) = delete;

    [[nodiscard]] bool valid() const noexcept { return m_fd >= 0; }
    [[nodiscard]] int get() const noexcept { return m_fd; }

    bool close() noexcept
    {
        const bool closed = m_fd < 0 || ::close(m_fd) == 0;
        m_fd = -1;
        return closed;
    }

private:
    int m_fd;
};

int openRetrying(const char* path, int flags, mode_t mode = 0) noexcept
{
    int fd = -1;
    do {
        fd = ::open(path, flags | O_CLOEXEC, mode);
    } while (fd < 0 && errno == EINTR);
    return fd;
}

std::error_code sync(int fd) noexcept
{
#if defined(__APPLE__)
    if (::fcntl(fd, F_FULLFSYNC) == 0) {
        return {};
    }
#endif
    return ::fsync(fd) == 0 ? std::error_code() : lastError();
}

std::error_code syncDirectoryOf(const std::filesystem::path& file) noexcept
{
    std::filesystem::path directory = file.parent_path();
    if (directory.empty()) {
        directory = ".";
    }
    const Descriptor handle(openRetrying(directory.c_str(), O_RDONLY | O_DIRECTORY));
    if (!handle.valid()) {
        return lastError();
    }
    const std::error_code error = sync(handle.get());
    return error.value() == EINVAL ? std::error_code() : error;
}

std::error_code writeAll(int fd, std::string_view contents) noexcept
{
    while (!contents.empty()) {
        const ssize_t written = ::write(fd, contents.data(), std::min<std::size_t>(contents.size(), 1u << 20));
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return written < 0 ? lastError() : std::make_error_code(std::errc::io_error);
        }
        contents.remove_prefix(static_cast<std::size_t>(written));
    }
    return {};
}

std::error_code renameOver(const std::filesystem::path& from, const std::filesystem::path& to) noexcept
{
    if (::rename(from.c_str(), to.c_str()) != 0) {
        return lastError();
    }
    return syncDirectoryOf(to);
}

}

bool fileBusy(const std::error_code& error) noexcept
{
    return error.category() == std::system_category() && (error.value() == EBUSY || error.value() == ETXTBSY);
}

bool fileMissing(const std::error_code& error) noexcept
{
    return error.category() == std::system_category() && (error.value() == ENOENT || error.value() == ENOTDIR);
}

std::error_code readFile(const std::filesystem::path& file, std::size_t mostBytes, std::string& contents,
                         WhenBusy) noexcept
{
    try {
        contents.clear();
        const Descriptor handle(openRetrying(file.c_str(), O_RDONLY));
        if (!handle.valid()) {
            return lastError();
        }
        struct stat status{};
        if (::fstat(handle.get(), &status) != 0) {
            return lastError();
        }
        if (S_ISDIR(status.st_mode)) {
            return std::make_error_code(std::errc::is_a_directory);
        }
        if (status.st_size < 0 || static_cast<unsigned long long>(status.st_size) > mostBytes) {
            return std::make_error_code(std::errc::file_too_large);
        }
        contents.resize(static_cast<std::size_t>(status.st_size));
        std::size_t done = 0;
        while (done < contents.size()) {
            const ssize_t got = ::read(handle.get(), contents.data() + done, contents.size() - done);
            if (got < 0 && errno == EINTR) {
                continue;
            }
            if (got < 0) {
                contents.clear();
                return lastError();
            }
            if (got == 0) {
                break;
            }
            done += static_cast<std::size_t>(got);
        }
        contents.resize(done);
        return {};
    } catch (const std::bad_alloc&) {
        contents.clear();
        return std::make_error_code(std::errc::not_enough_memory);
    }
}

std::error_code writeFileAtomically(const std::filesystem::path& target, std::string_view contents) noexcept
{
    try {
        std::filesystem::path temporary = target;
        temporary += ".tmp";
        Descriptor file(openRetrying(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
        if (!file.valid()) {
            return lastError();
        }
        std::error_code error = writeAll(file.get(), contents);
        if (!error) {
            error = sync(file.get());
        }
        if (!file.close() && !error) {
            error = lastError();
        }
        if (!error) {
            error = renameOver(temporary, target);
        }
        if (error) {
            ::unlink(temporary.c_str());
        }
        return error;
    } catch (const std::bad_alloc&) {
        return std::make_error_code(std::errc::not_enough_memory);
    }
}

std::error_code flushFile(const std::filesystem::path& file) noexcept
{
    const Descriptor handle(openRetrying(file.c_str(), O_RDONLY));
    if (!handle.valid()) {
        return lastError();
    }
    return sync(handle.get());
}

std::error_code flushTree(const std::filesystem::path& directory) noexcept
{
    try {
        std::error_code error;
        for (auto at = std::filesystem::recursive_directory_iterator(directory, error);
             !error && at != std::filesystem::recursive_directory_iterator(); at.increment(error)) {
            std::error_code kind;
            if (!at->is_regular_file(kind)) {
                continue;
            }
            if (const std::error_code flushed = flushFile(at->path())) {
                return flushed;
            }
        }
        return error;
    } catch (const std::bad_alloc&) {
        return std::make_error_code(std::errc::not_enough_memory);
    }
}

std::error_code replaceFile(const std::filesystem::path& from, const std::filesystem::path& to) noexcept
{
    if (const std::error_code flushed = flushFile(from)) {
        return flushed;
    }
    return renameOver(from, to);
}

std::error_code removeFile(const std::filesystem::path& file) noexcept
{
    if (::unlink(file.c_str()) == 0 || errno == ENOENT || errno == ENOTDIR) {
        return {};
    }
    return lastError();
}

}
