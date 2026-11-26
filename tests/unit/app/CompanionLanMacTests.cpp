// SPDX-License-Identifier: GPL-3.0-or-later
// CompanionLan on macOS: re-pairing over an existing pairing serves the first mirror link with the new key, and
// restarting the listener rebinds its port instead of bouncing to an ephemeral port.
#include "CompanionCipher.h"
#include "CompanionLan.h"

#include "fakes/FakeDeviceServer.h"
#include "fakes/RecordingListener.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <gs/platform/Crypto.h>
#include <gs/protocol/Codec.h>
#include <gs/session/CompanionMirror.h>
#include <gs/session/Session.h>

#include <doctest/doctest.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace gs;
using namespace gs::app;

namespace {

std::shared_ptr<const CompanionApk> dummyApk()
{
    auto loaded = std::make_shared<CompanionApk>();
    loaded->bytes = {0x50, 0x4B, 0x03, 0x04};
    return loaded;
}

template <typename T>
class Mailbox
{
public:
    void push(T item)
    {
        {
            const std::lock_guard lock(m_mutex);
            m_items.push_back(std::move(item));
        }
        m_changed.notify_all();
    }

    std::optional<T> pop(std::chrono::milliseconds timeout = 5s)
    {
        std::unique_lock lock(m_mutex);
        if (!m_changed.wait_for(lock, timeout, [this] { return !m_items.empty(); })) {
            return std::nullopt;
        }
        T item = std::move(m_items.front());
        m_items.erase(m_items.begin());
        return item;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_changed;
    std::vector<T> m_items;
};

class SocketClient
{
public:
    explicit SocketClient(std::uint16_t port)
    {
        m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        timeval timeout{.tv_sec = 5, .tv_usec = 0};
        ::setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        ::setsockopt(m_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(port);
        m_connected = ::connect(m_fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
    }

    ~SocketClient()
    {
        close();
    }

    SocketClient(const SocketClient&) = delete;
    SocketClient& operator=(const SocketClient&) = delete;

    [[nodiscard]] bool connected() const noexcept { return m_connected; }

    void close() noexcept
    {
        if (m_fd >= 0) {
            ::shutdown(m_fd, SHUT_RDWR);
            ::close(m_fd);
            m_fd = -1;
        }
    }

    bool writeAll(std::span<const std::uint8_t> bytes)
    {
        while (!bytes.empty()) {
            const ssize_t sent = ::send(m_fd, bytes.data(), bytes.size(), 0);
            if (sent <= 0) {
                return false;
            }
            bytes = bytes.subspan(static_cast<std::size_t>(sent));
        }
        return true;
    }

    std::vector<std::uint8_t> readExact(std::size_t count)
    {
        std::vector<std::uint8_t> out(count);
        std::size_t done = 0;
        while (done < count) {
            const ssize_t got = ::recv(m_fd, out.data() + done, count - done, 0);
            if (got <= 0) {
                out.resize(done);
                return out;
            }
            done += static_cast<std::size_t>(got);
        }
        return out;
    }

private:
    int m_fd = -1;
    bool m_connected = false;
};

}

TEST_CASE("CompanionLan: re-pairing over a stored pairing keeps the pc id and accepts the first mirror link with "
          "the new key")
{
    Mailbox<LanArrival> lanArrivals;
    Mailbox<MirrorArrival> mirrorArrivals;
    CompanionLan lan(dummyApk(), [&lanArrivals](LanArrival arrival) { lanArrivals.push(std::move(arrival)); }, 0, true);
    lan.setMirrorArrived([&mirrorArrivals](MirrorArrival arrival) { mirrorArrivals.push(std::move(arrival)); });
    REQUIRE(lan.listening());

    const LanId phoneId = newLanId();
    const LanPairing stalePairing{.phone = phoneId, .key = newLanKey(), .pc = newLanId()};
    const LanKey freshKey = newLanKey();
    const std::string token = newLanToken();
    lan.expect({LanExpectation{
        .serial = "aoa-RFGL71YMDHD",
        .token = token,
        .qrKey = freshKey,
        .paired = stalePairing,
        .announce = true,
    }});

    SocketClient gscl(lan.port());
    REQUIRE(gscl.connected());
    LanNonce helloNonce{};
    platform::randomBytes(helloNonce);
    const std::vector<std::uint8_t> lanHello = encodeLanHello({.token = token, .phone = phoneId, .nonce = helloNonce});
    REQUIRE(gscl.writeAll(lanHello));
    const std::vector<std::uint8_t> lanReplyBytes = gscl.readExact(kLanReplySize);
    const auto lanReply = decodeLanReply(lanReplyBytes);
    REQUIRE(lanReply.has_value());
    CHECK(lanReply->status == LanStatus::Accepted);
    CHECK(lanReply->pc == stalePairing.pc);

    const LanKey lanSession = lanSessionKey(freshKey, lanHello, lanReplyBytes);
    LanOpener lanOpener(lanSession, LanDirection::ToCompanion);
    LanSealer lanSealer(lanSession, LanDirection::ToComputer);

    // Just as LanConnector + CompanionService.mirrorOver do on Android, open GSCM in parallel right after GSCL's
    // reply arrives, before DeviceManager has saved or re-posted expectations.
    SocketClient gscm(lan.port());
    REQUIRE(gscm.connected());
    LanNonce mirrorNonce{};
    platform::randomBytes(mirrorNonce);
    std::vector<std::uint8_t> gscmHello = encodeLanHello({.phone = phoneId, .nonce = mirrorNonce});
    gscmHello[3] = 'M';
    std::vector<std::uint8_t> gscmReplyBytes;
    std::jthread mirrorHandshake([&] {
        REQUIRE(gscm.writeAll(gscmHello));
        gscmReplyBytes = gscm.readExact(kLanReplySize);
    });

    LanPrefix prefix{};
    const std::vector<std::uint8_t> head = gscl.readExact(4);
    REQUIRE(head.size() == 4);
    std::ranges::copy(head, prefix.begin());
    const auto length = LanOpener::recordLength(prefix);
    REQUIRE(length.has_value());
    const auto helloFrame = lanOpener.open(prefix, gscl.readExact(*length));
    REQUIRE(helloFrame.has_value());
    const auto welcomeFrame = protocol::encodeFrame(protocol::CompanionWelcome{});
    REQUIRE(welcomeFrame.has_value());
    REQUIRE(gscl.writeAll(lanSealer.seal(*welcomeFrame)));

    const auto pairedArrival = lanArrivals.pop();
    REQUIRE(pairedArrival.has_value());
    REQUIRE(pairedArrival->paired.has_value());
    CHECK(pairedArrival->paired->pc == stalePairing.pc);
    CHECK(pairedArrival->paired->key == freshKey);

    mirrorHandshake.join();
    const auto gscmReply = decodeLanReply(gscmReplyBytes);
    REQUIRE(gscmReply.has_value());
    CHECK(gscmReply->status == LanStatus::Accepted);
    CHECK(gscmReply->pc == stalePairing.pc);

    const auto mirrorArrival = mirrorArrivals.pop();
    REQUIRE(mirrorArrival.has_value());
    REQUIRE(mirrorArrival->pipe != nullptr);

    const LanKey expectedMirrorSession = mirrorSessionKey(freshKey, gscmHello, gscmReplyBytes);
    CHECK(mirrorArrival->token == mirrorToken(expectedMirrorSession));

    LanOpener mirrorOpener(expectedMirrorSession, LanDirection::ToCompanion, 0, true);
    const auto controlOpen = session::encodeMirrorControl(session::MirrorOp::Open, 0);
    REQUIRE(mirrorArrival->pipe->send(controlOpen));
    const std::vector<std::uint8_t> mHead = gscm.readExact(4);
    REQUIRE(mHead.size() == 4);
    std::ranges::copy(mHead, prefix.begin());
    const auto mLen = LanOpener::mirrorRecordLength(prefix);
    REQUIRE(mLen.has_value());
    const auto mPlain = mirrorOpener.open(prefix, gscm.readExact(*mLen));
    REQUIRE(mPlain.has_value());
    CHECK(*mPlain == std::vector<std::uint8_t>(controlOpen.begin(), controlOpen.end()));
}

TEST_CASE("CompanionLan: restarting the listener rebinds the same port even after an active client connection")
{
    std::uint16_t chosenPort = 0;
    {
        CompanionLan first(dummyApk(), {}, 0, true);
        REQUIRE(first.listening());
        chosenPort = first.port();
        REQUIRE(chosenPort != 0);
        SocketClient client(chosenPort);
        REQUIRE(client.connected());
        const std::vector<std::uint8_t> probe{'G', 'S', 'C', 'L'};
        REQUIRE(client.writeAll(probe));
    }
    CompanionLan second(dummyApk(), {}, chosenPort, true);
    REQUIRE(second.listening());
    CHECK(second.port() == chosenPort);
}
