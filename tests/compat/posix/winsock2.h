// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Test builds off Windows: the Winsock names the test fakes use, mapped onto BSD sockets.
#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <csignal>
#include <cstdint>

using SOCKET = int;
using WORD = std::uint16_t;
using BOOL = int;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

inline constexpr SOCKET INVALID_SOCKET = -1;
inline constexpr int SOCKET_ERROR = -1;
inline constexpr int SD_RECEIVE = SHUT_RD;
inline constexpr int SD_SEND = SHUT_WR;
inline constexpr int SD_BOTH = SHUT_RDWR;

struct WSADATA {
    WORD wVersion = 0;
};

constexpr WORD MAKEWORD(int low, int high) noexcept
{
    return static_cast<WORD>((low & 0xFF) | ((high & 0xFF) << 8));
}

inline int WSAStartup(WORD, WSADATA*) noexcept
{
    std::signal(SIGPIPE, SIG_IGN);
    return 0;
}

inline int WSACleanup() noexcept
{
    return 0;
}

inline int closesocket(SOCKET socket) noexcept
{
    ::shutdown(socket, SHUT_RDWR);
    return ::close(socket);
}

inline int getsockname(SOCKET socket, sockaddr* address, int* length) noexcept
{
    socklen_t size = static_cast<socklen_t>(*length);
    const int result = ::getsockname(socket, address, &size);
    *length = static_cast<int>(size);
    return result;
}
