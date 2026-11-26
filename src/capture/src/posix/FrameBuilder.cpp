// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Capture pictures off Windows: NV12, YUY2 and BGRA samples become frames in memory; MJPEG and H.264 from a
// capture card, and GPU textures, are not ported yet.
#include "gs/capture/FrameBuilder.h"

#include <gs/media/FrameUploader.h>

#include <mutex>
#include <vector>

namespace gs::capture {

void bgraToNv12(const std::uint8_t* pixels, std::ptrdiff_t pitch, int width, int height, std::uint8_t* luma,
                std::ptrdiff_t lumaPitch, std::uint8_t* chroma, std::ptrdiff_t chromaPitch) noexcept
{
    for (int y = 0; y < height; ++y) {
        const std::uint8_t* row = pixels + y * pitch;
        std::uint8_t* out = luma + y * lumaPitch;
        for (int x = 0; x < width; ++x) {
            const int blue = row[4 * x];
            const int green = row[4 * x + 1];
            const int red = row[4 * x + 2];
            out[x] = static_cast<std::uint8_t>(((47 * red + 157 * green + 16 * blue + 128) >> 8) + 16);
        }
    }
    for (int y = 0; y < (height + 1) / 2; ++y) {
        const std::uint8_t* top = pixels + (2 * y) * pitch;
        const std::uint8_t* bottom = pixels + std::min(2 * y + 1, height - 1) * pitch;
        std::uint8_t* out = chroma + y * chromaPitch;
        for (int x = 0; x < (width + 1) / 2; ++x) {
            int blue = 0;
            int green = 0;
            int red = 0;
            for (const std::uint8_t* row : {top, bottom}) {
                for (const int column : {2 * x, std::min(2 * x + 1, width - 1)}) {
                    blue += row[4 * column];
                    green += row[4 * column + 1];
                    red += row[4 * column + 2];
                }
            }
            blue = (blue + 2) / 4;
            green = (green + 2) / 4;
            red = (red + 2) / 4;
            out[2 * x] = static_cast<std::uint8_t>(((-26 * red - 86 * green + 112 * blue + 128) >> 8) + 128);
            out[2 * x + 1] = static_cast<std::uint8_t>(((112 * red - 102 * green - 10 * blue + 128) >> 8) + 128);
        }
    }
}


struct FrameBuilder::State {
    explicit State(Deliver handOver) : uploader(nullptr), deliver(std::move(handOver)) {}

    void hand(media::VideoFrame frame)
    {
        if (frame && deliver) {
            deliver(std::move(frame));
        }
    }

    void fail(std::string reason)
    {
        const std::scoped_lock lock(mutex);
        if (problem.empty()) {
            problem = std::move(reason);
        }
    }

    media::FrameUploader uploader;
    Deliver deliver;
    std::vector<std::uint8_t> scratch;
    int width = 0;
    int height = 0;
    std::uint16_t session = 0;
    mutable std::mutex mutex;
    std::string problem;
};

FrameBuilder::FrameBuilder(ID3D11Device*, Deliver deliver) : m_state(std::make_unique<State>(std::move(deliver))) {}

FrameBuilder::~FrameBuilder() = default;

void FrameBuilder::build(const CaptureSample& sample)
{
    State& state = *m_state;
    if (sample.width <= 0 || sample.height <= 0) {
        return;
    }
    if (sample.width != state.width || sample.height != state.height) {
        state.width = sample.width;
        state.height = sample.height;
        state.session = static_cast<std::uint16_t>(state.session % 0xFFFFU + 1U);
    }
    const media::FrameStamp stamp{.ptsUs = sample.ptsUs,
                                  .session = state.session,
                                  .receivedUs = sample.receivedUs,
                                  .color = sample.color};
    switch (sample.kind) {
    case SampleKind::Nv12:
        state.hand(state.uploader.nv12({sample.bytes.data(), sample.pitch, sample.bytes.data() + sample.chromaOffset,
                                        sample.pitch},
                                       sample.width, sample.height, stamp));
        return;
    case SampleKind::Yuy2:
        state.hand(state.uploader.yuy2(sample.bytes.data(), sample.pitch, sample.width, sample.height, stamp));
        return;
    case SampleKind::Bgra: {
        const auto rowBytes = static_cast<std::size_t>(sample.width) * 4;
        if (sample.pitch < static_cast<std::ptrdiff_t>(rowBytes) ||
            sample.bytes.size() <
                static_cast<std::size_t>(sample.pitch) * static_cast<std::size_t>(sample.height - 1) + rowBytes) {
            return;
        }
        const int lumaPitch = (sample.width + 1) / 2 * 2;
        const int rows = (sample.height + 1) / 2 * 2;
        state.scratch.resize(static_cast<std::size_t>(lumaPitch) * static_cast<std::size_t>(rows) * 3 / 2);
        std::uint8_t* luma = state.scratch.data();
        std::uint8_t* chroma = luma + static_cast<std::ptrdiff_t>(lumaPitch) * rows;
        bgraToNv12(sample.bytes.data(), sample.pitch, sample.width, sample.height, luma, lumaPitch, chroma, lumaPitch);
        state.hand(state.uploader.nv12({luma, lumaPitch, chroma, lumaPitch}, sample.width, sample.height, stamp));
        return;
    }
    case SampleKind::Mjpg:
    case SampleKind::H264:
    case SampleKind::Texture:
        state.fail("This capture card's picture format is not supported on this system yet");
        return;
    }
}

void FrameBuilder::reset()
{
    m_state->width = 0;
    m_state->height = 0;
}

std::string FrameBuilder::problem() const
{
    const std::scoped_lock lock(m_state->mutex);
    return m_state->problem;
}

std::uint64_t FrameBuilder::dropped() const noexcept
{
    return m_state->uploader.dropped();
}

std::uint16_t FrameBuilder::session() const noexcept
{
    return m_state->session;
}

}
