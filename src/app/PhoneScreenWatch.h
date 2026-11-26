// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
#pragma once

// Notices when the phone's screen goes off (sleep, or the Always On Display) while it is mirrored, for the
// "When the phone's screen turns off" setting. A thread of its own asks the phone's power service over adb every
// two seconds; the first answer is only remembered, so a screen that was already off reports nothing until it has
// been on. The GameScrcpy "screen off" command keeps the phone awake and so does not count.

#include <gs/adb/AdbClient.h>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace gs::app {

// Whether `dumpsys power` reports the phone awake (its screen on, or showing a screen saver): its wakefulness line
// says Awake or Dreaming; Asleep and Dozing are off. Nothing when the line is missing.
[[nodiscard]] std::optional<bool> phoneAwakeIn(std::string_view dumpsysPower) noexcept;

class PhoneScreenWatch
{
public:
    using WentOff = std::function<void()>;

    PhoneScreenWatch(adb::AdbClient adb, std::string serial, WentOff wentOff);
    ~PhoneScreenWatch();
    PhoneScreenWatch(const PhoneScreenWatch&) = delete;
    PhoneScreenWatch& operator=(const PhoneScreenWatch&) = delete;

private:
    void run(const std::stop_token& stop);

    adb::AdbClient m_adb;
    std::string m_serial;
    WentOff m_wentOff;
    std::jthread m_thread;
};

}
