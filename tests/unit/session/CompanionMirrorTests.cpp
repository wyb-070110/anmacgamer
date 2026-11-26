// SPDX-License-Identifier: GPL-3.0-or-later
#include "fakes/FakeDeviceServer.h"
#include "fakes/RecordingListener.h"

#include "gs/platform/TcpStream.h"
#include "gs/session/CompanionMirror.h"
#include "gs/session/Session.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace gs;
using namespace gs::session;
using namespace gs::session::test;
using namespace std::chrono_literals;
using nlohmann::json;

namespace {

std::vector<std::uint8_t> fromHex(const std::string& text)
{
    std::vector<std::uint8_t> bytes;
    for (std::size_t i = 0; i + 1 < text.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>(std::stoi(text.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

json mirrorVectors()
{
    std::ifstream file(std::filesystem::path(GS_PROTOCOL_VECTOR_DIR) / "companion" / "mirror.json", std::ios::binary);
    REQUIRE(file.good());
    const json document = json::parse(file);
    REQUIRE(document.at("format") == "gamescrcpy.companion-mirror-vectors");
    return document.at("vectors").at(0);
}

MirrorFrom senderOf(const json& record)
{
    return record.at("direction") == "toCompanion" ? MirrorFrom::Computer : MirrorFrom::Companion;
}

// Both ends of an in-memory link: what one end sends the other receives, record by record.
struct Wire {
    std::mutex mutex;
    std::condition_variable changed;
    std::array<std::deque<std::vector<std::uint8_t>>, 2> queues;
    bool closed = false;
};

class WireEnd final : public MirrorPipe
{
public:
    WireEnd(std::shared_ptr<Wire> wire, int side) : m_wire(std::move(wire)), m_side(side) {}
    ~WireEnd() override { close(); }

    bool send(std::span<const std::uint8_t> record) override
    {
        {
            const std::lock_guard lock(m_wire->mutex);
            if (m_wire->closed) {
                return false;
            }
            m_wire->queues[1 - m_side].emplace_back(record.begin(), record.end());
        }
        m_wire->changed.notify_all();
        return true;
    }

    bool receive(std::vector<std::uint8_t>& record) override
    {
        std::unique_lock lock(m_wire->mutex);
        m_wire->changed.wait(lock, [this] { return m_wire->closed || !m_wire->queues[m_side].empty(); });
        if (m_wire->queues[m_side].empty()) {
            return false;
        }
        record = std::move(m_wire->queues[m_side].front());
        m_wire->queues[m_side].pop_front();
        return true;
    }

    std::string endedReason() const override { return "the test link was cut"; }

    void close() noexcept override
    {
        {
            const std::lock_guard lock(m_wire->mutex);
            m_wire->closed = true;
        }
        m_wire->changed.notify_all();
    }

private:
    std::shared_ptr<Wire> m_wire;
    int m_side;
};

// The phone's end of the link: every channel the computer opens becomes a connection to the fake server, so the
// session runs the real protocol through the multiplexed records.
class FakeCompanion
{
public:
    FakeCompanion(FakeDeviceServer& server, std::shared_ptr<Wire> wire)
        : m_server(server), m_end(std::make_unique<WireEnd>(std::move(wire), 1))
    {
        m_reader = std::thread([this] { read(); });
    }

    ~FakeCompanion()
    {
        m_end->close();
        if (m_reader.joinable()) {
            m_reader.join();
        }
        std::map<std::uint8_t, std::shared_ptr<Bridge>> bridges;
        {
            const std::lock_guard lock(m_mutex);
            bridges.swap(m_bridges);
        }
        for (auto& [channel, bridge] : bridges) {
            bridge->tcp.interrupt();
            if (bridge->pump.joinable()) {
                bridge->pump.join();
            }
        }
        joinFinished();
    }

    FakeCompanion(const FakeCompanion&) = delete;
    FakeCompanion& operator=(const FakeCompanion&) = delete;

    void refuse(std::uint8_t channel)
    {
        const std::lock_guard lock(m_mutex);
        m_refused.push_back(channel);
    }

    void closeFromPhone(std::uint8_t channel)
    {
        std::shared_ptr<Bridge> bridge;
        {
            const std::lock_guard lock(m_mutex);
            const auto found = m_bridges.find(channel);
            if (found != m_bridges.end()) {
                bridge = found->second;
            }
        }
        if (bridge != nullptr) {
            bridge->tcp.interrupt();
        }
    }

    [[nodiscard]] std::vector<MirrorControl> ops()
    {
        const std::lock_guard lock(m_mutex);
        return m_ops;
    }

    bool waitForOps(std::size_t count, std::chrono::milliseconds timeout = 5s)
    {
        std::unique_lock lock(m_mutex);
        return m_changed.wait_for(lock, timeout, [&] { return m_ops.size() >= count; });
    }

private:
    struct Bridge {
        explicit Bridge(platform::TcpStream opened) : tcp(std::move(opened)) {}
        platform::TcpStream tcp;
        std::thread pump;
        bool closeSent = false;
        bool closeReceived = false;
    };

    void read()
    {
        std::vector<std::uint8_t> record;
        while (m_end->receive(record)) {
            const auto decoded = decodeMirrorRecord(record, MirrorFrom::Computer);
            REQUIRE(decoded.has_value());
            if (const auto* data = std::get_if<MirrorData>(&*decoded)) {
                std::shared_ptr<Bridge> bridge;
                {
                    const std::lock_guard lock(m_mutex);
                    const auto found = m_bridges.find(data->channel);
                    if (found != m_bridges.end()) {
                        bridge = found->second;
                    }
                }
                if (bridge != nullptr) {
                    static_cast<void>(bridge->tcp.writeAll(data->data));
                }
                continue;
            }
            const MirrorControl op = std::get<MirrorControl>(*decoded);
            {
                const std::lock_guard lock(m_mutex);
                m_ops.push_back(op);
            }
            m_changed.notify_all();
            if (op.op == MirrorOp::Open) {
                open(op.channel);
            } else {
                closedByComputer(op.channel);
            }
        }
    }

    void open(std::uint8_t channel)
    {
        bool refused = false;
        {
            const std::lock_guard lock(m_mutex);
            refused = std::ranges::find(m_refused, channel) != m_refused.end();
        }
        std::optional<platform::TcpStream> tcp;
        if (!refused) {
            tcp = platform::TcpStream::connectLoopback(m_server.port());
        }
        if (!tcp) {
            static_cast<void>(m_end->send(encodeMirrorControl(MirrorOp::Close, channel)));
            return;
        }
        auto bridge = std::make_shared<Bridge>(std::move(*tcp));
        {
            const std::lock_guard lock(m_mutex);
            m_bridges[channel] = bridge;
        }
        static_cast<void>(m_end->send(encodeMirrorControl(MirrorOp::Accept, channel)));
        bridge->pump = std::thread([this, bridge, channel] {
            std::array<std::uint8_t, 4096> buffer{};
            while (true) {
                const std::size_t received = bridge->tcp.readSome(buffer);
                if (received == 0) {
                    break;
                }
                const std::lock_guard lock(m_mutex);
                if (bridge->closeSent) {
                    break;
                }
                static_cast<void>(
                    m_end->send(encodeMirrorData(channel, std::span<const std::uint8_t>(buffer).first(received))));
            }
            finish(channel, bridge, false);
        });
    }

    void closedByComputer(std::uint8_t channel)
    {
        std::shared_ptr<Bridge> bridge;
        {
            const std::lock_guard lock(m_mutex);
            const auto found = m_bridges.find(channel);
            if (found == m_bridges.end()) {
                return;
            }
            bridge = found->second;
        }
        bridge->tcp.interrupt();
        finish(channel, bridge, true);
    }

    void finish(std::uint8_t channel, const std::shared_ptr<Bridge>& bridge, bool received)
    {
        {
            const std::lock_guard lock(m_mutex);
            if (received) {
                bridge->closeReceived = true;
            }
            if (!bridge->closeSent) {
                bridge->closeSent = true;
                static_cast<void>(m_end->send(encodeMirrorControl(MirrorOp::Close, channel)));
            }
            if (bridge->closeSent && bridge->closeReceived) {
                const auto found = m_bridges.find(channel);
                if (found != m_bridges.end() && found->second == bridge) {
                    m_finished.push_back(bridge);
                    m_bridges.erase(found);
                }
            }
        }
    }

    FakeDeviceServer& m_server;
    std::unique_ptr<WireEnd> m_end;
    std::mutex m_mutex;
    std::condition_variable m_changed;
    std::map<std::uint8_t, std::shared_ptr<Bridge>> m_bridges;
    std::vector<std::shared_ptr<Bridge>> m_finished;
    std::vector<std::uint8_t> m_refused;
    std::vector<MirrorControl> m_ops;
    std::thread m_reader;

public:
    void joinFinished()
    {
        std::vector<std::shared_ptr<Bridge>> finished;
        {
            const std::lock_guard lock(m_mutex);
            finished.swap(m_finished);
        }
        for (const auto& bridge : finished) {
            if (bridge->pump.joinable() && bridge->pump.get_id() != std::this_thread::get_id()) {
                bridge->pump.join();
            }
        }
    }
};

SessionOptions fastOptions()
{
    SessionOptions options;
    options.clientVersion = "test";
    options.linger = 5s;
    options.handshakeTimeout = 2s;
    options.pingInterval = 50ms;
    options.pongTimeout = 2s;
    options.stopGrace = 1s;
    protocol::ConfigureVideo configure;
    configure.codec = protocol::VideoCodec::H264;
    configure.maxSize = 1080;
    configure.bitrate = 8'000'000;
    options.video = configure;
    protocol::ConfigureAudio audio;
    audio.enabled = true;
    audio.bitrate = 128000;
    options.audio = audio;
    return options;
}

FakeServerOptions streamingServer()
{
    protocol::VideoSessionInfo info;
    info.frameWidth = 1080;
    info.frameHeight = 2400;
    info.displayWidth = 1080;
    info.displayHeight = 2400;
    const std::array<std::uint8_t, 8> config{0, 0, 0, 1, 0x67, 0x42, 0x00, 0x1F};
    std::vector<std::uint8_t> big(3 * kMirrorDataLimit / 2, 0x55);
    big[0] = 0;
    big[1] = 0;
    big[2] = 0;
    big[3] = 1;
    big[4] = 0x65;
    FakeServerOptions options;
    options.videoPackets.push_back(*protocol::encodeVideoSession(1, info));
    options.videoPackets.push_back(*protocol::encodeMediaConfig(protocol::MediaChannel::Video, 1, config));
    options.videoPackets.push_back(
        *protocol::encodeMediaFrame(protocol::MediaChannel::Video, 1, {true, false}, 1000, 8000, big));
    return options;
}

const Token kToken{0x38, 0x7c, 0xa6, 0x3d, 0x24, 0x29, 0xb6, 0xba, 0x84, 0x17, 0xe7, 0x48, 0xb6, 0x0a, 0x28, 0xc2};

struct Rig {
    Rig()
        : server(streamingServer()), mirror(std::make_shared<CompanionMirror>())
    {
        server.setToken(kToken);
        connect();
        session = std::make_unique<Session>(companionMirrorHost(mirror), fastOptions(), listener);
    }

    ~Rig()
    {
        session.reset();
        mirror->detach();
        if (phone != nullptr) {
            phone->joinFinished();
        }
        phone.reset();
    }

    Rig(const Rig&) = delete;
    Rig& operator=(const Rig&) = delete;

    void connect()
    {
        auto wire = std::make_shared<Wire>();
        if (phone != nullptr) {
            phone->joinFinished();
        }
        phone = std::make_unique<FakeCompanion>(server, wire);
        mirror->attach(std::make_unique<WireEnd>(wire, 0), kToken);
    }

    FakeDeviceServer server;
    std::shared_ptr<CompanionMirror> mirror;
    std::unique_ptr<FakeCompanion> phone;
    RecordingListener listener;
    std::unique_ptr<Session> session;
};

}

TEST_CASE("SPEC 11.10: the mirror records of the golden vectors decode, and encoding gives the same bytes back")
{
    const json vectors = mirrorVectors();
    for (const json& record : vectors.at("records")) {
        INFO(record.at("what").get<std::string>());
        const std::vector<std::uint8_t> mux = fromHex(record.at("mux"));
        const auto decoded = decodeMirrorRecord(mux, senderOf(record));
        REQUIRE(decoded.has_value());
        if (const auto* data = std::get_if<MirrorData>(&*decoded)) {
            CHECK(encodeMirrorData(data->channel, data->data) == mux);
        } else {
            const MirrorControl op = std::get<MirrorControl>(*decoded);
            const auto encoded = encodeMirrorControl(op.op, op.channel);
            CHECK(std::vector<std::uint8_t>(encoded.begin(), encoded.end()) == mux);
        }
    }
    for (const json& bad : vectors.at("malformedMux")) {
        INFO(bad.at("name").get<std::string>());
        const auto decoded = decodeMirrorRecord(fromHex(bad.at("hex")), senderOf(bad));
        REQUIRE_FALSE(decoded.has_value());
        CHECK(protocol::errorName(decoded.error()) == bad.at("error").get<std::string>());
    }
}

TEST_CASE("SPEC 11.10: a session runs the real protocol through the companion's mirror link, with the token the "
          "link carries, and splits nothing it should not")
{
    Rig rig;
    rig.session->start();
    REQUIRE(rig.listener.waitForPhase(SessionPhase::Streaming));
    CHECK(rig.listener.transports() == std::vector{TransportKind::Companion});
    REQUIRE(rig.listener.waitForVideo(3));
    const auto video = rig.listener.video();
    CHECK(video[0].kind == protocol::MediaKind::Session);
    CHECK(video[1].kind == protocol::MediaKind::Config);
    CHECK(video[2].kind == protocol::MediaKind::Frame);
    CHECK(video[2].size == 3 * kMirrorDataLimit / 2);
    CHECK(rig.server.handshakes() == 1);
    CHECK(rig.server.videoOpens() == 1);
    CHECK(rig.server.waitForAudioOpens(1));

    rig.session->stop();
    REQUIRE(rig.listener.waitForEnd());
    CHECK(rig.listener.phases().back() == SessionPhase::Stopped);
    CHECK(rig.server.waitForReceived<protocol::Goodbye>(1));
    REQUIRE(rig.phone->waitForOps(6));
    const auto ops = rig.phone->ops();
    CHECK(ops[0] == MirrorControl{MirrorOp::Open, 0});
    CHECK(std::ranges::count(ops, MirrorControl{MirrorOp::Close, 1}) == 1);
}

TEST_CASE("SPEC 11.10: when the companion refuses the control channel the session fails and says so")
{
    Rig rig;
    rig.phone->refuse(0);
    rig.session->start();
    REQUIRE(rig.listener.waitForEnd());
    const std::optional<SessionFailure> failure = rig.listener.failure();
    REQUIRE(failure.has_value());
    INFO(failure->detail);
    CHECK(failure->kind == FailureKind::ConnectFailed);
    CHECK(failure->detail.find("closed the control channel") != std::string::npos);
    CHECK(rig.server.connections() == 0);
}

TEST_CASE("SPEC 11.10: when the mirror link breaks the session waits for the companion's next link and resumes "
          "on it")
{
    Rig rig;
    rig.session->start();
    REQUIRE(rig.listener.waitForPhase(SessionPhase::Streaming));
    REQUIRE(rig.listener.waitForVideo(1));

    rig.mirror->detach();
    REQUIRE(rig.listener.waitForPhase(SessionPhase::Reconnecting));
    CHECK_FALSE(rig.mirror->attached());
    rig.connect();
    REQUIRE(rig.listener.waitForPhase(SessionPhase::Streaming, 2));
    CHECK(rig.listener.transports() == std::vector{TransportKind::Companion, TransportKind::Companion});
}

TEST_CASE("SPEC 11.10: a channel the companion closes reaches the session as a lost connection, and is opened again "
          "only after both ends closed it")
{
    Rig rig;
    rig.session->start();
    REQUIRE(rig.listener.waitForPhase(SessionPhase::Streaming));
    REQUIRE(rig.listener.waitForVideo(1));

    rig.phone->closeFromPhone(0);
    REQUIRE(rig.listener.waitForPhase(SessionPhase::Streaming, 2));
    const auto ops = rig.phone->ops();
    const auto reopened = std::ranges::find(ops.begin() + 1, ops.end(), MirrorControl{MirrorOp::Open, 0});
    REQUIRE(reopened != ops.end());
    CHECK(std::ranges::find(ops.begin(), reopened, MirrorControl{MirrorOp::Close, 0}) != reopened);
}

TEST_CASE("SPEC 11.10: when screen sharing stops on the phone, a clean ServerGoodbye ends the session as Stopped "
          "without reconnecting")
{
    Rig rig;
    rig.session->start();
    REQUIRE(rig.listener.waitForPhase(SessionPhase::Streaming));
    REQUIRE(rig.listener.waitForVideo(1));

    protocol::ServerGoodbye goodbye;
    goodbye.reason = protocol::ErrorCode::None;
    goodbye.message = "screen sharing stopped on the phone";
    REQUIRE(rig.server.sendControl(goodbye));
    rig.phone->closeFromPhone(0);

    REQUIRE(rig.listener.waitForEnd());
    const auto phases = rig.listener.phases();
    CHECK(phases.back() == SessionPhase::Stopped);
    CHECK(std::ranges::count(phases, SessionPhase::Reconnecting) == 0);
    CHECK(std::ranges::count(phases, SessionPhase::Failed) == 0);
    CHECK(rig.server.handshakes() == 1);
}
