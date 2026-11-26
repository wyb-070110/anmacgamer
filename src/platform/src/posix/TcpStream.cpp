// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// POSIX TcpStream (macOS and Linux): blocking loopback TCP with timeouts and an interrupt that wakes readers.
#include "gs/platform/TcpStream.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <utility>

namespace gs::platform {

namespace {

#if defined(MSG_NOSIGNAL)
constexpr int kSendFlags = MSG_NOSIGNAL;
#else
constexpr int kSendFlags = 0;
#endif

int native(std::uintptr_t socket) noexcept
{
    return static_cast<int>(socket);
}

void setTimeoutOption(std::uintptr_t socket, int option, std::chrono::milliseconds timeout) noexcept
{
    const auto milliseconds = timeout.count() < 0 ? 0 : timeout.count();
    timeval value{};
    value.tv_sec = static_cast<decltype(value.tv_sec)>(milliseconds / 1000);
    value.tv_usec = static_cast<decltype(value.tv_usec)>((milliseconds % 1000) * 1000);
    setsockopt(native(socket), SOL_SOCKET, option, &value, sizeof(value));
}

}

TcpStream::TcpStream(TcpStream&& other) noexcept : m_socket(std::exchange(other.m_socket, kInvalid)) {}

TcpStream& TcpStream::operator=(TcpStream&& other) noexcept
{
    if (this != &other) {
        close();
        m_socket = std::exchange(other.m_socket, kInvalid);
    }
    return *this;
}

TcpStream::~TcpStream()
{
    close();
}

void TcpStream::close() noexcept
{
    if (m_socket != kInvalid) {
        ::close(native(m_socket));
        m_socket = kInvalid;
    }
}

std::optional<TcpStream> TcpStream::connectLoopback(std::uint16_t port)
{
    const int handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (handle < 0) {
        return std::nullopt;
    }
    TcpStream stream(static_cast<std::uintptr_t>(handle));
    fcntl(handle, F_SETFD, FD_CLOEXEC);
#if defined(SO_NOSIGPIPE)
    const int noSigPipe = 1;
    setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe, sizeof(noSigPipe));
#endif
    const int noDelay = 1;
    setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int connected = 0;
    do {
        connected = ::connect(handle, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    } while (connected != 0 && errno == EINTR);
    if (connected != 0) {
        return std::nullopt;
    }
    return stream;
}

void TcpStream::setReadTimeout(std::chrono::milliseconds timeout) noexcept
{
    setTimeoutOption(m_socket, SO_RCVTIMEO, timeout);
}

void TcpStream::setWriteTimeout(std::chrono::milliseconds timeout) noexcept
{
    setTimeoutOption(m_socket, SO_SNDTIMEO, timeout);
}

bool TcpStream::writeAll(std::span<const std::uint8_t> bytes) noexcept
{
    while (!bytes.empty()) {
        const ssize_t sent = ::send(native(m_socket), bytes.data(), bytes.size(), kSendFlags);
        if (sent < 0 && errno == EINTR) {
            continue;
        }
        if (sent <= 0) {
            return false;
        }
        bytes = bytes.subspan(static_cast<std::size_t>(sent));
    }
    return true;
}

bool TcpStream::writeAll(std::string_view text) noexcept
{
    return writeAll(std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size()));
}

bool TcpStream::readExact(std::span<std::uint8_t> bytes) noexcept
{
    while (!bytes.empty()) {
        const std::size_t received = readSome(bytes);
        if (received == 0) {
            return false;
        }
        bytes = bytes.subspan(received);
    }
    return true;
}

std::size_t TcpStream::readSome(std::span<std::uint8_t> bytes) noexcept
{
    ssize_t received = 0;
    do {
        received = ::recv(native(m_socket), bytes.data(), bytes.size(), 0);
    } while (received < 0 && errno == EINTR);
    m_readTimedOut = received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK);
    return received > 0 ? static_cast<std::size_t>(received) : 0;
}

void TcpStream::interrupt() noexcept
{
    if (m_socket == kInvalid) {
        return;
    }
    ::shutdown(native(m_socket), SHUT_RDWR);
}

}
