// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Frames timed to the screen (FrameSlot with a known refresh): each one is due a steady time after the phone's
// timestamp, the newest due frame is shown and older ones skipped, a frame not due yet waits, and immediate
// presentation (low-latency mode) never waits.
#include <gs/render/FrameSlot.h>

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <cstdint>

using namespace gs;

namespace {

constexpr std::int64_t kRefreshUs = 16'667;

std::int64_t nowUs()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

media::VideoFrame frameAt(std::int64_t ptsUs, std::int64_t receivedUs)
{
    const std::array<std::uint8_t, 4> luma{};
    const std::array<std::uint8_t, 2> chroma{};
    return media::VideoFrame::copyNv12(luma.data(), 2, chroma.data(), 2, 2, 2, ptsUs, 1, receivedUs);
}

}

TEST_CASE("timed to a 60 Hz screen, the newest frame that is due is shown and the older one skipped")
{
    render::FrameSlot slot;
    slot.setDisplayIntervalUs(kRefreshUs);
    const std::int64_t now = nowUs();
    const std::int64_t pts = 1'000'000;
    slot.put(frameAt(pts, now - 20'000));
    slot.put(frameAt(pts + kRefreshUs, now));
    const media::VideoFrame shown = slot.take();
    REQUIRE(shown);
    CHECK(shown.ptsUs() == pts + kRefreshUs);
    CHECK(slot.replaced() == 1);
    CHECK_FALSE(slot.pending());
}

TEST_CASE("timed to the screen, a frame that came faster than usual waits for a later refresh")
{
    render::FrameSlot slot;
    slot.setDisplayIntervalUs(kRefreshUs);
    const std::int64_t now = nowUs();
    // Recent frames took 0 to 30 ms on the way: the hold is capped at one refresh. The newest arrived as fast as
    // the fastest, so it is due one refresh after it arrived, later than the coming refresh.
    std::int64_t pts = 1'000'000;
    for (int i = 0; i < 20; ++i, pts += kRefreshUs) {
        slot.put(frameAt(pts, pts + (i % 2 == 0 ? 0 : 30'000)));
        static_cast<void>(slot.take());
    }
    slot.put(frameAt(now, now));
    CHECK_FALSE(slot.take());
    CHECK(slot.pending());
}

TEST_CASE("presented immediately (low-latency mode), a frame is never held")
{
    render::FrameSlot slot;
    slot.setDisplayIntervalUs(0);
    const std::int64_t now = nowUs();
    slot.put(frameAt(now, now - 30'000));
    slot.put(frameAt(now + kRefreshUs, now));
    CHECK(slot.take());
}
