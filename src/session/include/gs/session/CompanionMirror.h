// SPDX-License-Identifier: GPL-3.0-or-later
// Mirroring through the companion app (SPEC 11.10): one link carries the session's channels as multiplexed records,
// and each channel is a virtual connection opened, accepted and closed by link operations.
#pragma once

#include "gs/base/Expected.h"
#include "gs/protocol/Errors.h"
#include "gs/session/StreamHost.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <variant>
#include <vector>

namespace gs::session {

inline constexpr std::size_t kMirrorHeaderSize = 5;
inline constexpr std::uint32_t kMirrorDataLimit = 1024 * 1024;
inline constexpr std::uint8_t kMirrorLinkChannel = 0xFF;
inline constexpr std::uint8_t kMirrorChannels = 4;
inline constexpr std::chrono::milliseconds kMirrorConsentTimeout{60000};

enum class MirrorOp : std::uint8_t { Open = 0, Accept = 1, Close = 2 };
enum class MirrorFrom : std::uint8_t { Computer, Companion };

struct MirrorData {
    std::uint8_t channel = 0;
    std::span<const std::uint8_t> data;
};

struct MirrorControl {
    MirrorOp op = MirrorOp::Open;
    std::uint8_t channel = 0;

    friend bool operator==(const MirrorControl&, const MirrorControl&) = default;
};

using MirrorRecord = std::variant<MirrorData, MirrorControl>;

[[nodiscard]] Expected<MirrorRecord, protocol::DecodeError> decodeMirrorRecord(std::span<const std::uint8_t> bytes,
                                                                               MirrorFrom from);
[[nodiscard]] std::vector<std::uint8_t> encodeMirrorData(std::uint8_t channel, std::span<const std::uint8_t> data);
[[nodiscard]] std::array<std::uint8_t, kMirrorHeaderSize + 2> encodeMirrorControl(MirrorOp op, std::uint8_t channel);

class MirrorPipe
{
public:
    virtual ~MirrorPipe() = default;

    [[nodiscard]] virtual bool send(std::span<const std::uint8_t> record) = 0;
    [[nodiscard]] virtual bool receive(std::vector<std::uint8_t>& record) = 0;
    [[nodiscard]] virtual std::string endedReason() const = 0;
    virtual void close() noexcept = 0;
};

class CompanionMirror final : public PhoneStreams
{
public:
    CompanionMirror();
    ~CompanionMirror() override;
    CompanionMirror(const CompanionMirror&) = delete;
    CompanionMirror& operator=(const CompanionMirror&) = delete;

    void attach(std::unique_ptr<MirrorPipe> pipe, const Token& token);
    void detach();
    [[nodiscard]] bool attached() const;

    [[nodiscard]] Expected<std::unique_ptr<ByteStream>, SessionFailure> open(StreamChannel channel,
                                                                            const LaunchParams& params,
                                                                            std::chrono::milliseconds timeout,
                                                                            std::stop_token stop) override;
    [[nodiscard]] Expected<void, SessionFailure> awaitChange(std::chrono::steady_clock::time_point deadline,
                                                             std::stop_token stop) override;
    [[nodiscard]] std::optional<Token> sessionToken() const override;
    [[nodiscard]] TransportKind kind() const noexcept override { return TransportKind::Companion; }

private:
    class Link;

    void replace(std::shared_ptr<Link> next);

    mutable std::mutex m_mutex;
    std::condition_variable_any m_changed;
    std::shared_ptr<Link> m_link;
    std::uint64_t m_generation = 0;
    std::uint64_t m_used = 0;
};

[[nodiscard]] std::unique_ptr<ServerHost> companionMirrorHost(std::shared_ptr<CompanionMirror> mirror);

}
