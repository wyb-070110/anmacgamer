// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
#pragma once

// "2026-11-24T06:50:33Z" to a time point: the one form of std::chrono::parse("%FT%TZ") the program reads, which
// libc++ (macOS) does not have.

#include <chrono>
#include <optional>
#include <string_view>

namespace gs {

[[nodiscard]] inline std::optional<std::chrono::sys_seconds> parseUtcSeconds(std::string_view text) noexcept
{
    if (text.size() != 20 || text[4] != '-' || text[7] != '-' || text[10] != 'T' || text[13] != ':' ||
        text[16] != ':' || text[19] != 'Z') {
        return std::nullopt;
    }
    const auto number = [text](std::size_t at, std::size_t count, int& out) {
        out = 0;
        for (std::size_t index = at; index < at + count; ++index) {
            if (text[index] < '0' || text[index] > '9') {
                return false;
            }
            out = out * 10 + (text[index] - '0');
        }
        return true;
    };
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!number(0, 4, year) || !number(5, 2, month) || !number(8, 2, day) || !number(11, 2, hour) ||
        !number(14, 2, minute) || !number(17, 2, second) || hour > 23 || minute > 59 || second > 60) {
        return std::nullopt;
    }
    const std::chrono::year_month_day date{std::chrono::year(year), std::chrono::month(static_cast<unsigned>(month)),
                                           std::chrono::day(static_cast<unsigned>(day))};
    if (!date.ok()) {
        return std::nullopt;
    }
    return std::chrono::sys_days(date) + std::chrono::hours(hour) + std::chrono::minutes(minute) +
           std::chrono::seconds(second);
}

}
