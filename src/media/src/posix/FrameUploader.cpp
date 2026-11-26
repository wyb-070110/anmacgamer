// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Capture frames off Windows: the pictures stay in memory as NV12 (what Windows does without a D3D11 device);
// the renderer uploads them. There are no D3D11 sources to copy from.
#include "gs/media/FrameUploader.h"

#include <algorithm>
#include <atomic>
#include <vector>

namespace gs::media {

namespace {

int evenUp(int value) noexcept
{
    return (value + 1) & ~1;
}

}

struct FrameUploader::State {
    int depth = kDefaultDepth;
    std::vector<std::uint8_t> scratch;
    std::atomic<std::uint64_t> dropped{0};
};

FrameUploader::FrameUploader(ID3D11Device*, int depth) : m_state(std::make_unique<State>())
{
    m_state->depth = std::clamp(depth, 2, 16);
}

FrameUploader::~FrameUploader() = default;

VideoFrame FrameUploader::nv12(const Nv12Planes& planes, int width, int height, const FrameStamp& stamp)
{
    if (planes.luma == nullptr || planes.chroma == nullptr || width <= 0 || height <= 0) {
        return {};
    }
    return VideoFrame::copyNv12(planes.luma, planes.lumaPitch, planes.chroma, planes.chromaPitch, width, height,
                                stamp.ptsUs, stamp.session, stamp.receivedUs, stamp.color);
}

VideoFrame FrameUploader::yuy2(const std::uint8_t* pixels, std::ptrdiff_t pitch, int width, int height,
                               const FrameStamp& stamp)
{
    if (pixels == nullptr || width < 2 || height < 1 || pitch < 2 * static_cast<std::ptrdiff_t>(width)) {
        return {};
    }
    State& state = *m_state;
    const int lumaPitch = evenUp(width);
    const int rows = evenUp(height);
    state.scratch.resize(static_cast<std::size_t>(lumaPitch) * static_cast<std::size_t>(rows) * 3 / 2);
    std::uint8_t* luma = state.scratch.data();
    std::uint8_t* chroma = luma + static_cast<std::ptrdiff_t>(lumaPitch) * rows;
    yuy2ToNv12(pixels, pitch, width, height, luma, lumaPitch, chroma, lumaPitch);
    return nv12({luma, lumaPitch, chroma, lumaPitch}, width, height, stamp);
}

VideoFrame FrameUploader::copy(ID3D11Texture2D*, unsigned, int, int, const FrameStamp&)
{
    return {};
}

VideoFrame FrameUploader::fitted(ID3D11Texture2D*, unsigned, int, int, int, int, const FrameStamp&)
{
    return {};
}

bool FrameUploader::drawsOnGpu()
{
    return false;
}

std::uint64_t FrameUploader::dropped() const noexcept
{
    return m_state->dropped.load(std::memory_order_relaxed);
}

void yuy2ToNv12(const std::uint8_t* pixels, std::ptrdiff_t pitch, int width, int height, std::uint8_t* luma,
                std::ptrdiff_t lumaPitch, std::uint8_t* chroma, std::ptrdiff_t chromaPitch) noexcept
{
    for (int y = 0; y < height; ++y) {
        const std::uint8_t* row = pixels + y * pitch;
        std::uint8_t* out = luma + y * lumaPitch;
        for (int x = 0; x < width; ++x) {
            out[x] = row[2 * x];
        }
    }
    for (int y = 0; y < (height + 1) / 2; ++y) {
        const std::uint8_t* upper = pixels + static_cast<std::ptrdiff_t>(2 * y) * pitch;
        const std::uint8_t* lower = pixels + static_cast<std::ptrdiff_t>(std::min(2 * y + 1, height - 1)) * pitch;
        std::uint8_t* out = chroma + y * chromaPitch;
        for (int x = 0; x < (width + 1) / 2; ++x) {
            out[2 * x] = static_cast<std::uint8_t>((upper[4 * x + 1] + lower[4 * x + 1] + 1) / 2);
            out[2 * x + 1] = static_cast<std::uint8_t>((upper[4 * x + 3] + lower[4 * x + 3] + 1) / 2);
        }
    }
}

}
