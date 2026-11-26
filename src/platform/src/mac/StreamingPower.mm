// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Keeping a streaming session responsive on macOS. While any session streams, the process declares a user
// initiated, latency critical activity, so macOS neither naps it (App Nap) nor coalesces its timers, nor sleeps
// while idle (Windows keeps the process out of EcoQoS). Windows also turns on the WLAN media streaming mode for
// wireless sessions; macOS has no such switch, so that request does nothing here.
#include "gs/platform/StreamingPower.h"

#include "gs/base/Log.h"

#import <Foundation/Foundation.h>

#include <atomic>
#include <mutex>

namespace gs::platform {

namespace {

constexpr std::string_view kLog = "power";

}

void StreamingPower::begin(std::string_view session, StreamingLink link) noexcept
{
    GS_ASSERT_ON_THREAD(m_thread);
    if (const auto found = m_sessions.find(session); found != m_sessions.end()) {
        if (found->second != link) {
            found->second = link;
            refresh();
        }
        return;
    }
    if (m_sessions.size() >= kMaxSessions) {
        return;
    }
    try {
        m_sessions.emplace(std::string(session), link);
    } catch (...) {
        return;
    }
    refresh();
}

void StreamingPower::end(std::string_view session) noexcept
{
    GS_ASSERT_ON_THREAD(m_thread);
    const auto found = m_sessions.find(session);
    if (found == m_sessions.end()) {
        return;
    }
    m_sessions.erase(found);
    refresh();
}

std::uint32_t StreamingPower::streamingSessions() const noexcept
{
    return static_cast<std::uint32_t>(m_sessions.size());
}

std::uint32_t StreamingPower::wirelessSessions() const noexcept
{
    std::uint32_t count = 0;
    for (const auto& entry : m_sessions) {
        if (entry.second == StreamingLink::Wireless) {
            ++count;
        }
    }
    return count;
}

void StreamingPower::refresh() noexcept
{
    bool wireless = false;
    for (const auto& entry : m_sessions) {
        if (entry.second == StreamingLink::Wireless) {
            wireless = true;
            break;
        }
    }
    if (wireless != m_mediaStreaming) {
        m_mediaStreaming = wireless;
        const std::uint32_t code = m_target.setMediaStreaming(wireless);
        if (code == 0) {
            log::debug(kLog, "media streaming mode {} requested", wireless ? "on" : "off");
        } else if (!m_mediaReported) {
            m_mediaReported = true;
            log::debug(kLog, "media streaming mode {} failed: {}", wireless ? "on" : "off", code);
        }
    }
    const bool speed = !m_sessions.empty();
    if (speed != m_executionSpeed) {
        m_executionSpeed = speed;
        const std::uint32_t code = m_target.setExecutionSpeed(speed);
        if (code == 0) {
            log::debug(kLog, "process power {}", speed ? "kept out of EcoQoS" : "left to the system");
        } else if (!m_speedReported) {
            m_speedReported = true;
            log::debug(kLog, "execution speed {} failed: {}", speed ? "on" : "off", code);
        }
    }
}

struct SystemStreamingPower::Worker {
    std::mutex mutex;
    id<NSObject> activity = nil;
};

SystemStreamingPower::SystemStreamingPower() noexcept = default;

SystemStreamingPower::~SystemStreamingPower()
{
    static_cast<void>(setExecutionSpeed(false));
}

std::uint32_t SystemStreamingPower::setMediaStreaming(bool) noexcept
{
    return 0;
}

std::uint32_t SystemStreamingPower::setExecutionSpeed(bool on) noexcept
{
    @autoreleasepool {
        try {
            if (m_worker == nullptr) {
                m_worker = std::make_unique<Worker>();
            }
        } catch (...) {
            return 1;
        }
        const std::lock_guard lock(m_worker->mutex);
        if (on && m_worker->activity == nil) {
            const NSActivityOptions options = NSActivityUserInitiated | NSActivityLatencyCritical;
            m_worker->activity = [[NSProcessInfo processInfo] beginActivityWithOptions:options
                                                                                reason:@"Streaming a phone"];
        } else if (!on && m_worker->activity != nil) {
            [[NSProcessInfo processInfo] endActivity:m_worker->activity];
            m_worker->activity = nil;
        }
        return 0;
    }
}

std::uint64_t SystemStreamingPower::mediaStreamingApplied() const noexcept
{
    return 0;
}

}
