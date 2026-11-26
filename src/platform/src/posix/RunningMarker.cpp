// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// POSIX running marker: an exclusive flock on a file in the temporary directory, held while the process
// runs. The lock goes with the process however it ends.
#include "gs/platform/RunningMarker.h"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

namespace gs::platform {

namespace {

std::string markerPath(std::wstring_view name)
{
    std::string ascii;
    for (const wchar_t c : name) {
        const bool plain = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') ||
                           c == L'-' || c == L'_';
        ascii.push_back(plain ? static_cast<char>(c) : '_');
    }
    return "/tmp/" + ascii + ".lock";
}

}

RunningMarker::RunningMarker(std::wstring_view name)
{
    try {
        const std::string path = markerPath(name);
        const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0666);
        if (fd < 0) {
            return;
        }
        ::fchmod(fd, 0666);
        if (::flock(fd, LOCK_SH | LOCK_NB) != 0) {
            ::close(fd);
            return;
        }
        m_local = new int(fd);
    } catch (...) {
    }
}

RunningMarker::~RunningMarker()
{
    m_global = nullptr;
    if (auto* fd = static_cast<int*>(m_local)) {
        ::close(*fd);
        delete fd;
    }
}

bool runningMarkerPresent(std::wstring_view name) noexcept
{
    try {
        const std::string path = markerPath(name);
        const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            return false;
        }
        const bool held = ::flock(fd, LOCK_EX | LOCK_NB) != 0;
        ::close(fd);
        return held;
    } catch (...) {
        return false;
    }
}

}
