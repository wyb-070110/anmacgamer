// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Capture-card encoding off Windows is not ported yet (Media Foundation there; VideoToolbox and AudioToolbox
// would serve on macOS). The encoders say so; the bitstream helpers are the same as on Windows.
#include "gs/capture/Encoders.h"

#include <cstring>

namespace gs::capture {

namespace {

constexpr const char* kNotPorted = "Recording from a capture card is not available on this system yet";

}

std::vector<std::uint8_t> h264ParameterSets(std::span<const std::uint8_t> accessUnit)
{
    static constexpr std::size_t kNone = static_cast<std::size_t>(-1);
    const auto startOf = [&accessUnit](std::size_t from) {
        for (std::size_t at = from; at + 3 <= accessUnit.size(); ++at) {
            if (accessUnit[at] != 0 || accessUnit[at + 1] != 0) {
                continue;
            }
            if (accessUnit[at + 2] == 1) {
                return std::pair{at, std::size_t{3}};
            }
            if (at + 4 <= accessUnit.size() && accessUnit[at + 2] == 0 && accessUnit[at + 3] == 1) {
                return std::pair{at, std::size_t{4}};
            }
        }
        return std::pair{kNone, std::size_t{0}};
    };
    std::vector<std::uint8_t> sets;
    auto [start, length] = startOf(0);
    while (start != kNone) {
        const std::size_t unit = start + length;
        const auto [next, nextLength] = startOf(unit);
        const std::size_t end = next == kNone ? accessUnit.size() : next;
        if (unit < end) {
            const unsigned type = accessUnit[unit] & 0x1FU;
            if (type == 7U || type == 8U) {
                constexpr std::array<std::uint8_t, 4> kStart{0, 0, 0, 1};
                sets.insert(sets.end(), kStart.begin(), kStart.end());
                sets.insert(sets.end(), accessUnit.begin() + static_cast<std::ptrdiff_t>(unit),
                            accessUnit.begin() + static_cast<std::ptrdiff_t>(end));
            }
        }
        start = next;
        length = nextLength;
    }
    return sets;
}

std::vector<std::uint8_t> aacConfig(int sampleRate, int channels)
{
    constexpr std::array<int, 13> kRates{96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                         22050, 16000, 12000, 11025, 8000,  7350};
    const auto found = std::ranges::find(kRates, sampleRate);
    const auto index = static_cast<unsigned>(found == kRates.end() ? 3 : found - kRates.begin());
    const unsigned value = (2U << 11U) | (index << 7U) | ((static_cast<unsigned>(channels) & 0xFU) << 3U);
    return {static_cast<std::uint8_t>(value >> 8U), static_cast<std::uint8_t>(value & 0xFFU)};
}

struct VideoEncoder::State {};

VideoEncoder::VideoEncoder(ID3D11Device*, Output) : m_state(std::make_unique<State>()) {}

VideoEncoder::~VideoEncoder() = default;

void VideoEncoder::encode(const media::VideoFrame&, const PixelBox&, std::int64_t) {}

std::string VideoEncoder::problem() const
{
    return kNotPorted;
}

bool VideoEncoder::hardware() const
{
    return false;
}

struct AudioEncoder::State {};

AudioEncoder::AudioEncoder(Output) : m_state(std::make_unique<State>()) {}

AudioEncoder::~AudioEncoder() = default;

void AudioEncoder::encode(std::span<const float>, int, int, std::int64_t) {}

std::string AudioEncoder::problem() const
{
    return kNotPorted;
}

}
