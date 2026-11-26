// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The capture thread on macOS: tasks, wakeups, and a capture target set and cleared. Keys and the mouse need the
// program in front with a key window, which a test run does not have; they are tried in the app.
#include "gs/platform/InputCapture.h"

#include <doctest/doctest.h>

#import <AppKit/AppKit.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

using namespace gs;
using namespace gs::platform;
using namespace std::chrono_literals;

namespace {

class Recorder final : public InputHandler
{
public:
    void key(const RawKey&, gs::Microseconds) override { keys.fetch_add(1); }
    void mouse(const RawMouse&, gs::Microseconds) override { mice.fetch_add(1); }
    void gamepad(const PadState&, gs::Microseconds) override { pads.fetch_add(1); }

    void wake(gs::Microseconds now) override
    {
        const std::scoped_lock lock(mutex);
        wakes.push_back(now);
        due.reset();
        changed.notify_all();
    }

    [[nodiscard]] std::optional<gs::Microseconds> nextWakeup() const override
    {
        const std::scoped_lock lock(mutex);
        return due;
    }

    void wakeAt(gs::Microseconds when)
    {
        const std::scoped_lock lock(mutex);
        due = when;
    }

    template <typename Predicate>
    bool waitFor(std::chrono::milliseconds limit, Predicate&& predicate)
    {
        std::unique_lock lock(mutex);
        return changed.wait_for(lock, limit, std::forward<Predicate>(predicate));
    }

    std::atomic<int> keys{0};
    std::atomic<int> mice{0};
    std::atomic<int> pads{0};
    mutable std::mutex mutex;
    std::condition_variable changed;
    std::optional<gs::Microseconds> due;
    std::vector<gs::Microseconds> wakes;
};

template <typename Predicate>
bool eventually(Predicate&& predicate, std::chrono::milliseconds limit = 2000ms)
{
    const auto deadline = std::chrono::steady_clock::now() + limit;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
        std::this_thread::sleep_for(2ms);
    }
    return true;
}

}

TEST_CASE("on macOS the capture thread runs posted tasks in order on its own thread and stops cleanly")
{
    Recorder recorder;
    std::vector<int> order;
    std::mutex mutex;
    std::thread::id worker;
    {
        InputCapture capture(recorder);
        REQUIRE(capture.started());
        for (int i = 0; i < 50; ++i) {
            capture.post([&, i] {
                const std::scoped_lock lock(mutex);
                order.push_back(i);
                worker = std::this_thread::get_id();
            });
        }
        CHECK(eventually([&] {
            const std::scoped_lock lock(mutex);
            return order.size() == 50;
        }));
    }
    CHECK(std::is_sorted(order.begin(), order.end()));
    CHECK(worker != std::this_thread::get_id());
    CHECK(worker != std::thread::id{});
}

TEST_CASE("on macOS the capture thread wakes the handler at its next wakeup and stays idle without one")
{
    Recorder recorder;
    InputCapture capture(recorder);
    const gs::Microseconds start = InputCapture::now();
    capture.post([&] { recorder.wakeAt(start + gs::Microseconds{20'000}); });
    CHECK(recorder.waitFor(2000ms, [&] { return !recorder.wakes.empty(); }));
    {
        const std::scoped_lock lock(recorder.mutex);
        REQUIRE(recorder.wakes.size() == 1);
        CHECK(recorder.wakes.front() >= start + gs::Microseconds{20'000});
    }
    std::this_thread::sleep_for(100ms);
    const std::scoped_lock lock(recorder.mutex);
    CHECK(recorder.wakes.size() == 1);
}

TEST_CASE("on macOS a capture target can be set, changed and cleared, and the capture still stops cleanly")
{
    @autoreleasepool {
        NSView* first = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];
        NSView* second = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];
        Recorder recorder;
        {
            InputCapture capture(recorder);
            capture.setCaptureTarget(reinterpret_cast<std::uintptr_t>((__bridge void*)first));
            capture.setCaptureTarget(reinterpret_cast<std::uintptr_t>((__bridge void*)second));
            capture.setCaptureTarget(0);
            capture.setCaptureTarget(reinterpret_cast<std::uintptr_t>((__bridge void*)first));
            std::atomic<bool> ran{false};
            capture.post([&ran] { ran = true; });
            CHECK(eventually([&ran] { return ran.load(); }));
        }
        CHECK(recorder.keys.load() == 0);
        CHECK(recorder.mice.load() == 0);
    }
}
