// SPDX-License-Identifier: GPL-3.0-or-later
// Mirroring through the companion app (SPEC 11.10): the computer's end of the multiplexed link.
#include "gs/session/CompanionMirror.h"

#include <gs/base/Log.h>
#include "gs/platform/Thread.h"

#include <algorithm>
#include <format>
#include <thread>
#include <utility>

namespace gs::session {

namespace {

using SteadyClock = std::chrono::steady_clock;

SessionFailure failure(FailureKind kind, std::string detail)
{
    SessionFailure result;
    result.kind = kind;
    result.detail = std::move(detail);
    return result;
}

std::uint32_t readU32(std::span<const std::uint8_t> bytes) noexcept
{
    return (std::uint32_t{bytes[0]} << 24U) | (std::uint32_t{bytes[1]} << 16U) | (std::uint32_t{bytes[2]} << 8U) |
           std::uint32_t{bytes[3]};
}

void writeHeader(std::span<std::uint8_t> out, std::uint8_t channel, std::uint32_t length) noexcept
{
    out[0] = channel;
    out[1] = static_cast<std::uint8_t>(length >> 24U);
    out[2] = static_cast<std::uint8_t>(length >> 16U);
    out[3] = static_cast<std::uint8_t>(length >> 8U);
    out[4] = static_cast<std::uint8_t>(length);
}

std::string_view channelName(std::uint8_t channel) noexcept
{
    return streamChannelName(static_cast<StreamChannel>(channel));
}

}

Expected<MirrorRecord, protocol::DecodeError> decodeMirrorRecord(std::span<const std::uint8_t> bytes, MirrorFrom from)
{
    using protocol::DecodeError;
    if (bytes.size() < kMirrorHeaderSize) {
        return makeUnexpected(DecodeError::Truncated);
    }
    const std::uint8_t channel = bytes[0];
    const std::uint32_t length = readU32(bytes.subspan(1, 4));
    const std::span<const std::uint8_t> rest = bytes.subspan(kMirrorHeaderSize);
    if (channel != kMirrorLinkChannel && channel >= kMirrorChannels) {
        return makeUnexpected(DecodeError::InvalidEnum);
    }
    if (channel != kMirrorLinkChannel) {
        if (length == 0) {
            return makeUnexpected(DecodeError::OutOfRange);
        }
        if (length > kMirrorDataLimit) {
            return makeUnexpected(DecodeError::LengthExceeded);
        }
        if (rest.size() < length) {
            return makeUnexpected(DecodeError::Truncated);
        }
        if (rest.size() > length) {
            return makeUnexpected(DecodeError::TrailingBytes);
        }
        return MirrorRecord{MirrorData{channel, rest}};
    }
    if (length != 2) {
        return makeUnexpected(DecodeError::OutOfRange);
    }
    if (rest.size() < 2) {
        return makeUnexpected(DecodeError::Truncated);
    }
    if (rest[0] > static_cast<std::uint8_t>(MirrorOp::Close) || rest[1] >= kMirrorChannels) {
        return makeUnexpected(DecodeError::InvalidEnum);
    }
    const auto op = static_cast<MirrorOp>(rest[0]);
    if ((op == MirrorOp::Open && from != MirrorFrom::Computer) ||
        (op == MirrorOp::Accept && from != MirrorFrom::Companion)) {
        return makeUnexpected(DecodeError::WrongDirection);
    }
    if (rest.size() > 2) {
        return makeUnexpected(DecodeError::TrailingBytes);
    }
    return MirrorRecord{MirrorControl{op, rest[1]}};
}

std::vector<std::uint8_t> encodeMirrorData(std::uint8_t channel, std::span<const std::uint8_t> data)
{
    std::vector<std::uint8_t> record(kMirrorHeaderSize + data.size());
    writeHeader(record, channel, static_cast<std::uint32_t>(data.size()));
    std::ranges::copy(data, record.begin() + kMirrorHeaderSize);
    return record;
}

std::array<std::uint8_t, kMirrorHeaderSize + 2> encodeMirrorControl(MirrorOp op, std::uint8_t channel)
{
    std::array<std::uint8_t, kMirrorHeaderSize + 2> record{};
    writeHeader(record, kMirrorLinkChannel, 2);
    record[5] = static_cast<std::uint8_t>(op);
    record[6] = channel;
    return record;
}

// One link: the reader thread demultiplexes what the companion sends into the channels' transport streams. A channel
// may be opened again only after both ends have closed it (SPEC 11.10, virtual connections, rule 3).
class CompanionMirror::Link final : public std::enable_shared_from_this<Link>
{
public:
    enum class State : std::uint8_t { Closed, Opening, Open, Closing, Answering };

    Link(std::unique_ptr<MirrorPipe> pipe, const Token& token) : m_pipe(std::move(pipe)), m_token(token) {}

    ~Link()
    {
        m_pipe->close();
        if (m_reader.joinable()) {
            m_reader.join();
        }
    }

    Link(const Link&) = delete;
    Link& operator=(const Link&) = delete;

    void start()
    {
        m_reader = std::thread([this] {
            platform::setCurrentThreadName("gs-mirror-link");
            read();
        });
    }

    void end()
    {
        m_pipe->close();
    }

    [[nodiscard]] const Token& token() const noexcept { return m_token; }

    [[nodiscard]] bool alive() const
    {
        const std::lock_guard lock(m_mutex);
        return !m_ended;
    }

    Expected<std::shared_ptr<TransportStream>, SessionFailure> begin(std::uint8_t channel, SteadyClock::time_point deadline,
                                                                     const std::stop_token& stop)
    {
        std::weak_ptr<Link> weak = weak_from_this();
        auto stream = std::make_shared<TransportStream>(
            [weak, channel](std::span<const std::uint8_t> bytes) {
                const std::shared_ptr<Link> link = weak.lock();
                return link != nullptr && link->sendData(channel, bytes) ? StreamSend::Taken : StreamSend::Failed;
            },
            [weak, channel] {
                if (const std::shared_ptr<Link> link = weak.lock()) {
                    link->closeFromComputer(channel);
                }
            });
        {
            std::unique_lock lock(m_mutex);
            m_changed.wait_until(lock, stop, deadline,
                                 [&] { return m_ended || m_channels[channel].state == State::Closed; });
            if (m_ended) {
                return makeUnexpected(failure(FailureKind::ConnectFailed,
                                              std::format("the companion's mirror link ended: {}", m_reason)));
            }
            if (m_channels[channel].state != State::Closed) {
                return makeUnexpected(failure(stop.stop_requested() ? FailureKind::LaunchFailed : FailureKind::Timeout,
                                              std::format("the companion did not finish closing the {} channel",
                                                          channelName(channel))));
            }
            m_channels[channel].state = State::Opening;
            m_channels[channel].stream = stream;
        }
        if (!m_pipe->send(encodeMirrorControl(MirrorOp::Open, channel))) {
            stream->closed("the mirror link broke asking for the channel");
        }
        return stream;
    }

private:
    struct Channel {
        State state = State::Closed;
        std::shared_ptr<TransportStream> stream;
    };

    bool sendData(std::uint8_t channel, std::span<const std::uint8_t> bytes)
    {
        {
            const std::lock_guard lock(m_mutex);
            if (m_ended || m_channels[channel].state != State::Open) {
                return false;
            }
        }
        while (!bytes.empty()) {
            const std::span<const std::uint8_t> piece = bytes.first(std::min<std::size_t>(bytes.size(), kMirrorDataLimit));
            if (!m_pipe->send(encodeMirrorData(channel, piece))) {
                return false;
            }
            bytes = bytes.subspan(piece.size());
        }
        return true;
    }

    void closeFromComputer(std::uint8_t channel)
    {
        {
            const std::lock_guard lock(m_mutex);
            Channel& open = m_channels[channel];
            if (m_ended || (open.state != State::Opening && open.state != State::Open)) {
                return;
            }
            open.state = State::Closing;
            open.stream.reset();
        }
        static_cast<void>(m_pipe->send(encodeMirrorControl(MirrorOp::Close, channel)));
    }

    void read()
    {
        std::vector<std::uint8_t> record;
        std::string reason;
        while (true) {
            if (!m_pipe->receive(record)) {
                reason = m_pipe->endedReason();
                break;
            }
            const auto decoded = decodeMirrorRecord(record, MirrorFrom::Companion);
            if (!decoded) {
                reason = std::format("the companion sent a malformed mirror record ({})",
                                     protocol::errorName(decoded.error()));
                break;
            }
            if (const auto* data = std::get_if<MirrorData>(&*decoded)) {
                std::shared_ptr<TransportStream> stream;
                {
                    const std::lock_guard lock(m_mutex);
                    if (m_channels[data->channel].state == State::Open) {
                        stream = m_channels[data->channel].stream;
                    }
                }
                if (stream != nullptr) {
                    stream->deliver(data->data);
                }
                continue;
            }
            control(std::get<MirrorControl>(*decoded));
        }
        m_pipe->close();
        std::vector<std::shared_ptr<TransportStream>> streams;
        {
            const std::lock_guard lock(m_mutex);
            m_ended = true;
            m_reason = reason.empty() ? std::string("the connection was closed") : reason;
            for (Channel& channel : m_channels) {
                if (channel.stream != nullptr) {
                    streams.push_back(std::move(channel.stream));
                }
                channel.state = State::Closed;
            }
        }
        m_changed.notify_all();
        log::info("session", "the companion's mirror link ended: {}", reason.empty() ? "the connection was closed" : reason);
        for (const std::shared_ptr<TransportStream>& stream : streams) {
            stream->closed(m_reason);
        }
    }

    void control(const MirrorControl& op)
    {
        Channel& channel = m_channels[op.channel];
        std::shared_ptr<TransportStream> stream;
        bool answer = false;
        {
            const std::lock_guard lock(m_mutex);
            if (op.op == MirrorOp::Accept) {
                if (channel.state == State::Opening) {
                    channel.state = State::Open;
                    stream = channel.stream;
                }
            } else if (op.op == MirrorOp::Close) {
                if (channel.state == State::Opening || channel.state == State::Open) {
                    stream = std::move(channel.stream);
                    channel.state = State::Answering;
                    answer = true;
                } else if (channel.state == State::Closing) {
                    channel.state = State::Closed;
                }
            }
        }
        if (op.op == MirrorOp::Accept) {
            if (stream != nullptr) {
                stream->opened();
            }
            return;
        }
        if (stream != nullptr) {
            stream->closed(std::format("the companion closed the {} channel", channelName(op.channel)));
        }
        if (answer) {
            static_cast<void>(m_pipe->send(encodeMirrorControl(MirrorOp::Close, op.channel)));
            const std::lock_guard lock(m_mutex);
            if (channel.state == State::Answering) {
                channel.state = State::Closed;
            }
        }
        m_changed.notify_all();
    }

    const std::unique_ptr<MirrorPipe> m_pipe;
    const Token m_token;
    mutable std::mutex m_mutex;
    std::condition_variable_any m_changed;
    std::array<Channel, kMirrorChannels> m_channels{};
    bool m_ended = false;
    std::string m_reason;
    std::thread m_reader;
};

CompanionMirror::CompanionMirror() = default;

CompanionMirror::~CompanionMirror()
{
    replace(nullptr);
}

void CompanionMirror::attach(std::unique_ptr<MirrorPipe> pipe, const Token& token)
{
    auto link = std::make_shared<Link>(std::move(pipe), token);
    link->start();
    replace(std::move(link));
}

void CompanionMirror::detach()
{
    replace(nullptr);
}

bool CompanionMirror::attached() const
{
    std::shared_ptr<Link> link;
    {
        const std::lock_guard lock(m_mutex);
        link = m_link;
    }
    return link != nullptr && link->alive();
}

void CompanionMirror::replace(std::shared_ptr<Link> next)
{
    std::shared_ptr<Link> previous;
    {
        const std::lock_guard lock(m_mutex);
        previous = std::exchange(m_link, std::move(next));
        if (m_link != nullptr) {
            ++m_generation;
        }
    }
    m_changed.notify_all();
    if (previous != nullptr) {
        previous->end();
    }
}

Expected<std::unique_ptr<ByteStream>, SessionFailure> CompanionMirror::open(StreamChannel channel, const LaunchParams&,
                                                                           std::chrono::milliseconds timeout,
                                                                           std::stop_token stop)
{
    std::shared_ptr<Link> link;
    {
        const std::lock_guard lock(m_mutex);
        link = m_link;
        m_used = m_generation;
    }
    if (link == nullptr || !link->alive()) {
        return makeUnexpected(failure(FailureKind::ConnectFailed, "the companion is not connected for mirroring"));
    }
    const auto wait = channel == StreamChannel::Control ? std::max(timeout, kMirrorConsentTimeout) : timeout;
    auto stream = link->begin(static_cast<std::uint8_t>(channel), SteadyClock::now() + timeout, stop);
    if (!stream) {
        return makeUnexpected(std::move(stream.error()));
    }
    return awaitStream(std::move(*stream), "the companion", wait, stop);
}

Expected<void, SessionFailure> CompanionMirror::awaitChange(SteadyClock::time_point deadline, std::stop_token stop)
{
    std::unique_lock lock(m_mutex);
    if (!m_changed.wait_until(lock, stop, deadline,
                              [this] { return m_link != nullptr && m_generation != m_used; })) {
        return makeUnexpected(failure(FailureKind::ConnectionLost, "the companion did not come back for mirroring"));
    }
    return {};
}

std::optional<Token> CompanionMirror::sessionToken() const
{
    const std::lock_guard lock(m_mutex);
    if (m_link == nullptr) {
        return std::nullopt;
    }
    return m_link->token();
}

std::unique_ptr<ServerHost> companionMirrorHost(std::shared_ptr<CompanionMirror> mirror)
{
    return std::make_unique<StreamServerHost>(std::move(mirror));
}

}
