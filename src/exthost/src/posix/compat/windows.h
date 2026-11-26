// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// gs-exthost off Windows: the Win32 names exthost (and the native test DLLs) use, mapped onto POSIX in
// posix/Compat.cpp, so Channel, MediaRing, ImageSection, TapSection, InProcess, Server and most of Hosted stay as
// they are upstream.
//
// A HANDLE is one integer that means the same in the app and in gs-exthost, as on Windows, because the child
// inherits the descriptors with the same numbers:
//   tag 0  nullptr, or a pointer to an object only this process uses (process, timer, job, library)
//   tag 1  event: a pipe, read end in bits 0-23, write end in bits 24-47, bit 48 set for manual reset
//   tag 2  file mapping: a shared memory descriptor in bits 0-23
//   tag 3  watch: a descriptor that turns readable, for good, when the other end closes (the parent going away)
//   tag 4  remote: a handle the app sent over the bridge (bits 0-31 its number), turned into the local one on use
//   tag 5  file: a plain descriptor in bits 0-23
// DuplicateHandle between the app and gs-exthost passes descriptors over a Unix socket (posix/Bridge.h).
#pragma once

#include <cstddef>
#include <cstdint>

using HANDLE = void*;
using PVOID = void*;
using LPVOID = void*;
using PSID = void*;
using HMODULE = void*;
using FARPROC = void (*)();
using DWORD = std::uint32_t;
using UINT = unsigned int;
using BOOL = int;
using BYTE = std::uint8_t;
using LONG = std::int32_t;
using LONGLONG = std::int64_t;
using SIZE_T = std::size_t;
using ULONG_PTR = std::uintptr_t;
using HRESULT = std::int32_t;

union LARGE_INTEGER {
    struct {
        std::uint32_t LowPart;
        std::int32_t HighPart;
    } u;
    std::int64_t QuadPart;
};

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#define INVALID_HANDLE_VALUE (reinterpret_cast<HANDLE>(~std::uintptr_t{0}))

inline constexpr DWORD INFINITE = 0xFFFFFFFFu;
inline constexpr DWORD WAIT_OBJECT_0 = 0;
inline constexpr DWORD WAIT_ABANDONED = 0x80;
inline constexpr DWORD WAIT_TIMEOUT = 258;
inline constexpr DWORD WAIT_FAILED = 0xFFFFFFFFu;
inline constexpr DWORD MAXIMUM_WAIT_OBJECTS = 64;
inline constexpr DWORD STILL_ACTIVE = 259;

inline constexpr DWORD ERROR_SUCCESS = 0;
inline constexpr DWORD ERROR_FILE_NOT_FOUND = 2;
inline constexpr DWORD ERROR_PATH_NOT_FOUND = 3;
inline constexpr DWORD ERROR_ACCESS_DENIED = 5;
inline constexpr DWORD ERROR_INVALID_HANDLE = 6;
inline constexpr DWORD ERROR_NOT_ENOUGH_MEMORY = 8;
inline constexpr DWORD ERROR_NOT_SUPPORTED = 50;
inline constexpr DWORD ERROR_INVALID_PARAMETER = 87;
inline constexpr DWORD ERROR_ALREADY_EXISTS = 183;
inline constexpr DWORD ERROR_MOD_NOT_FOUND = 126;
inline constexpr DWORD ERROR_PROC_NOT_FOUND = 127;
inline constexpr DWORD ERROR_TIMEOUT = 1460;

inline constexpr DWORD PAGE_READONLY = 0x02;
inline constexpr DWORD PAGE_READWRITE = 0x04;
inline constexpr DWORD FILE_MAP_WRITE = 0x0002;
inline constexpr DWORD FILE_MAP_READ = 0x0004;
inline constexpr DWORD FILE_MAP_ALL_ACCESS = 0x000F001F;

inline constexpr DWORD DUPLICATE_CLOSE_SOURCE = 0x1;
inline constexpr DWORD DUPLICATE_SAME_ACCESS = 0x2;
inline constexpr DWORD SYNCHRONIZE = 0x00100000;
inline constexpr DWORD PROCESS_DUP_HANDLE = 0x0040;
inline constexpr DWORD PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;

inline constexpr DWORD CREATE_WAITABLE_TIMER_MANUAL_RESET = 0x1;
inline constexpr DWORD CREATE_WAITABLE_TIMER_HIGH_RESOLUTION = 0x2;
inline constexpr DWORD TIMER_ALL_ACCESS = 0x001F0003;

inline constexpr UINT SEM_FAILCRITICALERRORS = 0x0001;
inline constexpr UINT SEM_NOGPFAULTERRORBOX = 0x0002;
inline constexpr UINT SEM_NOOPENFILEERRORBOX = 0x8000;

inline constexpr DWORD LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR = 0x00000100;
inline constexpr DWORD LOAD_LIBRARY_SEARCH_SYSTEM32 = 0x00000800;

inline constexpr DWORD GENERIC_READ = 0x80000000u;
inline constexpr DWORD GENERIC_WRITE = 0x40000000u;
inline constexpr DWORD FILE_SHARE_READ = 0x1;
inline constexpr DWORD FILE_SHARE_WRITE = 0x2;
inline constexpr DWORD FILE_SHARE_DELETE = 0x4;
inline constexpr DWORD CREATE_NEW = 1;
inline constexpr DWORD CREATE_ALWAYS = 2;
inline constexpr DWORD OPEN_EXISTING = 3;
inline constexpr DWORD OPEN_ALWAYS = 4;
inline constexpr DWORD FILE_ATTRIBUTE_NORMAL = 0x80;

inline constexpr UINT CP_UTF8 = 65001;
inline constexpr DWORD EXCEPTION_ACCESS_VIOLATION = 0xC0000005u;
inline constexpr DWORD EXCEPTION_NONCONTINUABLE = 0x1;

DWORD GetLastError() noexcept;
void SetLastError(DWORD error) noexcept;
BOOL CloseHandle(HANDLE handle) noexcept;

HANDLE CreateEventW(void* attributes, BOOL manualReset, BOOL initialState, const wchar_t* name) noexcept;
BOOL SetEvent(HANDLE event) noexcept;
BOOL ResetEvent(HANDLE event) noexcept;
DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds) noexcept;
DWORD WaitForMultipleObjects(DWORD count, const HANDLE* handles, BOOL waitAll, DWORD milliseconds) noexcept;

HANDLE CreateFileMappingW(HANDLE file, void* attributes, DWORD protect, DWORD sizeHigh, DWORD sizeLow,
                          const wchar_t* name) noexcept;
LPVOID MapViewOfFile(HANDLE mapping, DWORD access, DWORD offsetHigh, DWORD offsetLow, SIZE_T bytes) noexcept;
BOOL UnmapViewOfFile(const void* view) noexcept;

HANDLE GetCurrentProcess() noexcept;
DWORD GetCurrentProcessId() noexcept;
DWORD GetCurrentThreadId() noexcept;
HANDLE OpenProcess(DWORD access, BOOL inherit, DWORD processId) noexcept;
BOOL TerminateProcess(HANDLE process, UINT exitCode) noexcept;
BOOL GetExitCodeProcess(HANDLE process, DWORD* exitCode) noexcept;
BOOL DuplicateHandle(HANDLE sourceProcess, HANDLE source, HANDLE targetProcess, HANDLE* target, DWORD access,
                     BOOL inherit, DWORD options) noexcept;

HANDLE CreateWaitableTimerExW(void* attributes, const wchar_t* name, DWORD flags, DWORD access) noexcept;
BOOL SetWaitableTimer(HANDLE timer, const LARGE_INTEGER* due, LONG period, void* routine, void* argument,
                      BOOL resume) noexcept;
BOOL CancelWaitableTimer(HANDLE timer) noexcept;

BOOL QueryPerformanceCounter(LARGE_INTEGER* counter) noexcept;
BOOL QueryPerformanceFrequency(LARGE_INTEGER* frequency) noexcept;
void Sleep(DWORD milliseconds) noexcept; // source-rule-allow R9: Win32 Sleep declaration for the POSIX exthost compatibility layer
UINT SetErrorMode(UINT mode) noexcept;

HMODULE LoadLibraryExW(const wchar_t* path, HANDLE file, DWORD flags) noexcept;
// A std::filesystem::path's c_str() is char off Windows.
HMODULE LoadLibraryExW(const char* path, HANDLE file, DWORD flags) noexcept;
FARPROC GetProcAddress(HMODULE library, const char* name) noexcept;
BOOL FreeLibrary(HMODULE library) noexcept;

DWORD GetEnvironmentVariableW(const wchar_t* name, wchar_t* buffer, DWORD size) noexcept;
DWORD GetEnvironmentVariableA(const char* name, char* buffer, DWORD size) noexcept;
const wchar_t* GetCommandLineW() noexcept;
int MultiByteToWideChar(UINT codePage, DWORD flags, const char* text, int bytes, wchar_t* wide, int size) noexcept;

HANDLE CreateFileW(const wchar_t* path, DWORD access, DWORD share, void* attributes, DWORD disposition, DWORD flags,
                   HANDLE templateFile) noexcept;
BOOL ReadFile(HANDLE file, void* buffer, DWORD wanted, DWORD* read, void* overlapped) noexcept;
BOOL WriteFile(HANDLE file, const void* buffer, DWORD size, DWORD* written, void* overlapped) noexcept;

[[noreturn]] void RaiseException(DWORD code, DWORD flags, DWORD count, const ULONG_PTR* arguments) noexcept;
