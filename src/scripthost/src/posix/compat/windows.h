// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// gs-scripthost off Windows: the few Win32 names RemoteLogicHost.cpp and Pipe.cpp use, mapped onto POSIX so
// those files stay as they are upstream. A pipe HANDLE is its descriptor plus one, a process HANDLE a
// ScriptProcess made by the POSIX launch().
#pragma once

#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

using HANDLE = void*;
using DWORD = std::uint32_t;
using UINT = unsigned int;
using BOOL = int;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

inline constexpr DWORD INFINITE = 0xFFFFFFFFu;
inline constexpr DWORD WAIT_OBJECT_0 = 0;
inline constexpr DWORD WAIT_TIMEOUT = 258;
inline constexpr DWORD WAIT_FAILED = 0xFFFFFFFFu;
inline constexpr DWORD STILL_ACTIVE = 259;
inline constexpr UINT SEM_FAILCRITICALERRORS = 1;
inline constexpr UINT SEM_NOGPFAULTERRORBOX = 2;
inline constexpr UINT SEM_NOOPENFILEERRORBOX = 0x8000;

namespace gs::scripthost::posix {

struct ScriptProcess {
    pid_t pid = 0;
    bool reaped = false;
    int status = 0;
};

inline constexpr std::uintptr_t kMostDescriptorHandle = 1u << 20;

inline HANDLE descriptorHandle(int fd) noexcept
{
    return reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(fd) + 1);
}

inline bool isDescriptor(HANDLE handle) noexcept
{
    return handle != nullptr && reinterpret_cast<std::uintptr_t>(handle) <= kMostDescriptorHandle;
}

inline int descriptorOf(HANDLE handle) noexcept
{
    return static_cast<int>(reinterpret_cast<std::uintptr_t>(handle) - 1);
}

inline bool reap(ScriptProcess& process, bool block) noexcept
{
    if (process.reaped) {
        return true;
    }
    for (;;) {
        const pid_t done = ::waitpid(process.pid, &process.status, block ? 0 : WNOHANG);
        if (done == process.pid) {
            process.reaped = true;
            return true;
        }
        if (done < 0 && errno == EINTR) {
            continue;
        }
        if (done < 0) {
            process.reaped = true;
            process.status = 0;
            return true;
        }
        return false;
    }
}

}

inline thread_local DWORD g_gsLastError = 0;

// The Win32 stand-ins below are static: the extension host defines functions of the same names (src/exthost/src/posix/
// Compat.cpp) for its own handles, and with external linkage the linker kept one copy for both, so closing a logic
// pipe ran the extension host's CloseHandle and crashed.

static inline DWORD GetLastError() noexcept
{
    return g_gsLastError != 0 ? g_gsLastError : static_cast<DWORD>(errno);
}

static inline BOOL CloseHandle(HANDLE handle) noexcept
{
    using namespace gs::scripthost::posix;
    if (handle == nullptr) {
        return FALSE;
    }
    if (isDescriptor(handle)) {
        return ::close(descriptorOf(handle)) == 0 ? TRUE : FALSE;
    }
    auto* process = static_cast<ScriptProcess*>(handle);
    reap(*process, false);
    delete process;
    return TRUE;
}

static inline BOOL ReadFile(HANDLE handle, void* buffer, DWORD wanted, DWORD* read, void*) noexcept
{
    using namespace gs::scripthost::posix;
    *read = 0;
    if (!isDescriptor(handle)) {
        return FALSE;
    }
    for (;;) {
        const ssize_t got = ::read(descriptorOf(handle), buffer, wanted);
        if (got < 0 && errno == EINTR) {
            continue;
        }
        if (got < 0) {
            return FALSE;
        }
        *read = static_cast<DWORD>(got);
        return TRUE;
    }
}

static inline BOOL WriteFile(HANDLE handle, const void* buffer, DWORD size, DWORD* written, void*) noexcept
{
    using namespace gs::scripthost::posix;
    *written = 0;
    if (!isDescriptor(handle)) {
        return FALSE;
    }
    for (;;) {
        const ssize_t sent = ::write(descriptorOf(handle), buffer, size);
        if (sent < 0 && errno == EINTR) {
            continue;
        }
        if (sent < 0) {
            return FALSE;
        }
        *written = static_cast<DWORD>(sent);
        return TRUE;
    }
}

static inline BOOL TerminateProcess(HANDLE handle, UINT) noexcept
{
    using namespace gs::scripthost::posix;
    if (handle == nullptr || isDescriptor(handle)) {
        return FALSE;
    }
    auto* process = static_cast<ScriptProcess*>(handle);
    if (process->reaped) {
        return TRUE;
    }
    return ::kill(process->pid, SIGKILL) == 0 ? TRUE : FALSE;
}

static inline DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds) noexcept
{
    using namespace gs::scripthost::posix;
    if (handle == nullptr || isDescriptor(handle)) {
        return WAIT_FAILED;
    }
    auto* process = static_cast<ScriptProcess*>(handle);
    if (milliseconds == INFINITE) {
        reap(*process, true);
        return WAIT_OBJECT_0;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    while (!reap(*process, false)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return WAIT_TIMEOUT;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // source-rule-allow R9: POSIX has no waitpid with a timeout
    }
    return WAIT_OBJECT_0;
}

static inline BOOL GetExitCodeProcess(HANDLE handle, DWORD* code) noexcept
{
    using namespace gs::scripthost::posix;
    if (handle == nullptr || isDescriptor(handle)) {
        return FALSE;
    }
    auto* process = static_cast<ScriptProcess*>(handle);
    if (!reap(*process, false)) {
        *code = STILL_ACTIVE;
        return TRUE;
    }
    if (WIFEXITED(process->status)) {
        *code = static_cast<DWORD>(WEXITSTATUS(process->status));
    } else if (WIFSIGNALED(process->status)) {
        *code = 0xC0000000u | static_cast<DWORD>(WTERMSIG(process->status));
    } else {
        *code = 0;
    }
    return TRUE;
}

static inline HANDLE GetCurrentThread() noexcept
{
    return nullptr;
}

static inline long SetThreadDescription(HANDLE, const wchar_t* name) noexcept
{
    std::string narrow;
    for (; name != nullptr && *name != L'\0'; ++name) {
        narrow.push_back(static_cast<char>(*name));
    }
#if defined(__APPLE__)
    pthread_setname_np(narrow.c_str());
#else
    narrow.resize(std::min<std::size_t>(narrow.size(), 15));
    pthread_setname_np(pthread_self(), narrow.c_str());
#endif
    return 0;
}

static inline UINT SetErrorMode(UINT) noexcept
{
    return 0;
}

static inline DWORD GetModuleFileNameW(void*, wchar_t* buffer, DWORD size) noexcept
{
    std::error_code error;
    std::filesystem::path self;
#if defined(__APPLE__)
    char raw[MAX_PATH]{};
    std::uint32_t length = sizeof(raw);
    if (_NSGetExecutablePath(raw, &length) == 0) {
        self = std::filesystem::canonical(raw, error);
    }
#else
    self = std::filesystem::read_symlink("/proc/self/exe", error);
#endif
    const std::wstring wide = self.wstring();
    if (wide.empty() || wide.size() + 1 > size) {
        return wide.empty() ? 0 : size;
    }
    std::wmemcpy(buffer, wide.c_str(), wide.size() + 1);
    return static_cast<DWORD>(wide.size());
}
