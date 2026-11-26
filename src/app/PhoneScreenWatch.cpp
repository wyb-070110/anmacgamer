// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
#include "PhoneScreenWatch.h"

#include <gs/base/Log.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <utility>

namespace gs::app {

namespace {

constexpr std::string_view kLog = "screen";
constexpr std::string_view kQuery = "dumpsys power | grep -m1 mWakefulness=";
constexpr std::chrono::seconds kPeriod{2};

}

std::optional<bool> phoneAwakeIn(std::string_view dumpsysPower) noexcept
{
    constexpr std::string_view kKey = "mWakefulness=";
    const std::size_t at = dumpsysPower.find(kKey);
    if (at == std::string_view::npos) {
        return std::nullopt;
    }
    const std::string_view state = dumpsysPower.substr(at + kKey.size());
    if (state.starts_with("Awake") || state.starts_with("Dreaming")) {
        return true;
    }
    if (state.starts_with("Asleep") || state.starts_with("Dozing")) {
        return false;
    }
    return std::nullopt;
}

PhoneScreenWatch::PhoneScreenWatch(adb::AdbClient adb, std::string serial, WentOff wentOff)
    : m_adb(std::move(adb)), m_serial(std::move(serial)), m_wentOff(std::move(wentOff)),
      m_thread([this](const std::stop_token& stop) { run(stop); })
{
}

PhoneScreenWatch::~PhoneScreenWatch()
{
    m_thread.request_stop();
}

void PhoneScreenWatch::run(const std::stop_token& stop)
{
    std::mutex mutex;
    std::condition_variable_any idle;
    std::optional<bool> awake;
    while (!stop.stop_requested()) {
        const auto answer = m_adb.shell(m_serial, kQuery);
        if (answer) {
            if (const std::optional<bool> now = phoneAwakeIn(answer->out)) {
                if (awake == true && !*now) {
                    log::info(kLog, "{}: the phone's screen went off", m_serial);
                    m_wentOff();
                }
                awake = now;
            }
        }
        std::unique_lock lock(mutex);
        idle.wait_for(lock, stop, kPeriod, [] { return false; });
    }
}

}
