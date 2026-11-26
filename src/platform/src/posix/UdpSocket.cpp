// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// POSIX UdpSocket (macOS and Linux). Video marking sets DSCP CS5 (and the video service type on macOS);
// interrupt also sends the socket an empty datagram, since shutdown does not wake an unconnected UDP receive
// on macOS.
#include "gs/platform/UdpSocket.h"

#include "gs/base/Log.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <format>
#include <mutex>
#include <string_view>
#include <utility>

namespace gs::platform {

namespace {

constexpr std::string_view kLog = "network";
constexpr int kVideoTrafficClass = 0x28 << 2;

int native(std::uintptr_t socket) noexcept
{
    return static_cast<int>(socket);
}

socklen_t toSockaddr(const UdpEndpoint& endpoint, sockaddr_storage& storage) noexcept
{
    std::memset(&storage, 0, sizeof(storage));
    if (endpoint.v6) {
        auto& address = reinterpret_cast<sockaddr_in6&>(storage);
        address.sin6_family = AF_INET6;
        address.sin6_port = htons(endpoint.port);
        std::memcpy(&address.sin6_addr, endpoint.address.data(), 16);
        return sizeof(sockaddr_in6);
    }
    auto& address = reinterpret_cast<sockaddr_in&>(storage);
    address.sin_family = AF_INET;
    address.sin_port = htons(endpoint.port);
    std::memcpy(&address.sin_addr, endpoint.address.data(), 4);
    return sizeof(sockaddr_in);
}

UdpEndpoint fromSockaddr(const sockaddr_storage& storage) noexcept
{
    UdpEndpoint endpoint;
    if (storage.ss_family == AF_INET6) {
        const auto& address = reinterpret_cast<const sockaddr_in6&>(storage);
        endpoint.v6 = true;
        endpoint.port = ntohs(address.sin6_port);
        std::memcpy(endpoint.address.data(), &address.sin6_addr, 16);
    } else {
        const auto& address = reinterpret_cast<const sockaddr_in&>(storage);
        endpoint.port = ntohs(address.sin_port);
        std::memcpy(endpoint.address.data(), &address.sin_addr, 4);
    }
    return endpoint;
}

bool markSocket(int socket, bool v6) noexcept
{
    bool marked = v6 ? setsockopt(socket, IPPROTO_IPV6, IPV6_TCLASS, &kVideoTrafficClass, sizeof(kVideoTrafficClass)) == 0
                     : setsockopt(socket, IPPROTO_IP, IP_TOS, &kVideoTrafficClass, sizeof(kVideoTrafficClass)) == 0;
#if defined(SO_NET_SERVICE_TYPE)
    const int serviceType = NET_SERVICE_TYPE_VI;
    marked = setsockopt(socket, SOL_SOCKET, SO_NET_SERVICE_TYPE, &serviceType, sizeof(serviceType)) == 0 || marked;
#endif
    if (!marked && log::isEnabled(log::Level::Debug)) {
        log::debug(kLog, "UDP packets are not marked as video traffic (errno {})", errno);
    }
    return marked;
}

}

std::optional<UdpEndpoint> UdpEndpoint::parse(std::string_view host, std::uint16_t port)
{
    if (host.empty() || host.size() >= INET6_ADDRSTRLEN) {
        return std::nullopt;
    }
    char text[INET6_ADDRSTRLEN]{};
    std::memcpy(text, host.data(), host.size());
    UdpEndpoint endpoint;
    endpoint.port = port;
    in_addr v4{};
    if (inet_pton(AF_INET, text, &v4) == 1) {
        std::memcpy(endpoint.address.data(), &v4, 4);
        return endpoint;
    }
    in6_addr v6{};
    if (inet_pton(AF_INET6, text, &v6) == 1) {
        endpoint.v6 = true;
        std::memcpy(endpoint.address.data(), &v6, 16);
        return endpoint;
    }
    return std::nullopt;
}

UdpEndpoint UdpEndpoint::loopback(std::uint16_t port) noexcept
{
    UdpEndpoint endpoint;
    endpoint.address = {127, 0, 0, 1};
    endpoint.port = port;
    return endpoint;
}

UdpEndpoint UdpEndpoint::any(bool v6, std::uint16_t port) noexcept
{
    UdpEndpoint endpoint;
    endpoint.v6 = v6;
    endpoint.port = port;
    return endpoint;
}

std::string UdpEndpoint::toString() const
{
    char text[INET6_ADDRSTRLEN]{};
    if (v6) {
        in6_addr raw{};
        std::memcpy(&raw, address.data(), 16);
        inet_ntop(AF_INET6, &raw, text, sizeof(text));
        return std::format("[{}]:{}", static_cast<const char*>(text), port);
    }
    in_addr raw{};
    std::memcpy(&raw, address.data(), 4);
    inet_ntop(AF_INET, &raw, text, sizeof(text));
    return std::format("{}:{}", static_cast<const char*>(text), port);
}

struct UdpSocket::VideoFlow {
    std::mutex mutex;
    Marking state = Marking::Pending;
};

UdpSocket::UdpSocket(UdpSocket&& other) noexcept
    : m_socket(std::exchange(other.m_socket, kInvalid)), m_v6(other.m_v6),
      m_interrupted(other.m_interrupted.load(std::memory_order_relaxed)),
      m_videoFlow(std::move(other.m_videoFlow))
{
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept
{
    if (this != &other) {
        close();
        m_socket = std::exchange(other.m_socket, kInvalid);
        m_v6 = other.m_v6;
        m_interrupted.store(other.m_interrupted.load(std::memory_order_relaxed), std::memory_order_relaxed);
        m_videoFlow = std::move(other.m_videoFlow);
    }
    return *this;
}

UdpSocket::~UdpSocket()
{
    close();
}

void UdpSocket::close() noexcept
{
    m_videoFlow.reset();
    if (m_socket != kInvalid) {
        ::close(native(m_socket));
        m_socket = kInvalid;
    }
}

std::optional<UdpSocket> UdpSocket::bind(const UdpEndpoint& local)
{
    const int handle = ::socket(local.v6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (handle < 0) {
        return std::nullopt;
    }
    UdpSocket socket(static_cast<std::uintptr_t>(handle), local.v6);
    fcntl(handle, F_SETFD, FD_CLOEXEC);
    sockaddr_storage storage{};
    const socklen_t length = toSockaddr(local, storage);
    if (::bind(handle, reinterpret_cast<const sockaddr*>(&storage), length) != 0) {
        return std::nullopt;
    }
    return socket;
}

std::uint16_t UdpSocket::localPort() const noexcept
{
    sockaddr_storage storage{};
    socklen_t length = sizeof(storage);
    if (m_socket == kInvalid || getsockname(native(m_socket), reinterpret_cast<sockaddr*>(&storage), &length) != 0) {
        return 0;
    }
    return fromSockaddr(storage).port;
}

void UdpSocket::setReceiveTimeout(std::chrono::milliseconds timeout) noexcept
{
    const auto milliseconds = timeout.count() < 0 ? 0 : timeout.count();
    timeval value{};
    value.tv_sec = static_cast<decltype(value.tv_sec)>(milliseconds / 1000);
    value.tv_usec = static_cast<decltype(value.tv_usec)>((milliseconds % 1000) * 1000);
    setsockopt(native(m_socket), SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));
}

bool UdpSocket::setReceiveBufferSize(std::size_t bytes) noexcept
{
    const int requested = static_cast<int>(std::min<std::size_t>(bytes, INT_MAX));
#if defined(SO_RCVBUFFORCE)
    if (setsockopt(native(m_socket), SOL_SOCKET, SO_RCVBUFFORCE, &requested, sizeof(requested)) == 0) {
        return true;
    }
#endif
    return setsockopt(native(m_socket), SOL_SOCKET, SO_RCVBUF, &requested, sizeof(requested)) == 0;
}

bool UdpSocket::markAsVideo(const UdpEndpoint& to) noexcept
{
    if (m_socket == kInvalid || m_videoFlow != nullptr || to.v6 != m_v6) {
        return false;
    }
    try {
        auto flow = std::make_shared<VideoFlow>();
        flow->state = markSocket(native(m_socket), m_v6) ? Marking::Video : Marking::Failed;
        m_videoFlow = std::move(flow);
        return true;
    } catch (...) {
        return false;
    }
}

UdpSocket::Marking UdpSocket::marking() const noexcept
{
    if (m_videoFlow == nullptr) {
        return Marking::None;
    }
    const std::lock_guard lock(m_videoFlow->mutex);
    return m_videoFlow->state;
}

bool UdpSocket::sendTo(const UdpEndpoint& to, std::span<const std::uint8_t> datagram) noexcept
{
    if (m_interrupted.load(std::memory_order_acquire) || to.v6 != m_v6) {
        return false;
    }
    sockaddr_storage storage{};
    const socklen_t length = toSockaddr(to, storage);
    ssize_t sent = 0;
    do {
        sent = ::sendto(native(m_socket), datagram.data(), datagram.size(), 0,
                        reinterpret_cast<const sockaddr*>(&storage), length);
    } while (sent < 0 && errno == EINTR);
    if (sent < 0) {
        const int error = errno;
        log::debug(kLog, "UDP send to {} failed: {} (errno {})", to.toString(), std::strerror(error), error);
    }
    return sent == static_cast<ssize_t>(datagram.size());
}

std::optional<UdpSocket::Received> UdpSocket::receive(std::span<std::uint8_t> buffer) noexcept
{
    m_receiveTimedOut = false;
    while (!m_interrupted.load(std::memory_order_acquire)) {
        sockaddr_storage storage{};
        iovec part{buffer.data(), buffer.size()};
        msghdr message{};
        message.msg_name = &storage;
        message.msg_namelen = sizeof(storage);
        message.msg_iov = &part;
        message.msg_iovlen = 1;
        const ssize_t received = ::recvmsg(native(m_socket), &message, 0);
        if (m_interrupted.load(std::memory_order_acquire)) {
            break;
        }
        if (received >= 0) {
            if ((message.msg_flags & MSG_TRUNC) != 0) {
                continue;
            }
            return Received{static_cast<std::size_t>(received), fromSockaddr(storage)};
        }
        if (errno == EINTR || errno == ECONNREFUSED) {
            continue;
        }
        m_receiveTimedOut = errno == EAGAIN || errno == EWOULDBLOCK;
        break;
    }
    return std::nullopt;
}

void UdpSocket::interrupt() noexcept
{
    m_interrupted.store(true, std::memory_order_release);
    if (m_socket == kInvalid) {
        return;
    }
    ::shutdown(native(m_socket), SHUT_RDWR);
    sockaddr_storage local{};
    socklen_t length = sizeof(local);
    if (getsockname(native(m_socket), reinterpret_cast<sockaddr*>(&local), &length) != 0) {
        return;
    }
    UdpEndpoint self = fromSockaddr(local);
    if (std::all_of(self.address.begin(), self.address.end(), [](std::uint8_t byte) { return byte == 0; })) {
        if (self.v6) {
            self.address[15] = 1;
        } else {
            self.address = {127, 0, 0, 1};
        }
    }
    sockaddr_storage target{};
    const socklen_t targetLength = toSockaddr(self, target);
    const int waker = ::socket(self.v6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (waker >= 0) {
        ::sendto(waker, nullptr, 0, 0, reinterpret_cast<const sockaddr*>(&target), targetLength);
        ::close(waker);
    }
}

}
