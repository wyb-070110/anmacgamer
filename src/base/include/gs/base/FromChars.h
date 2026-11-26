// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// std::from_chars for integers and floating point everywhere: libc++ before LLVM 20 (Apple Clang included)
// has no floating-point from_chars, so there it parses with strtod in the C locale, keeping from_chars' rules.
// Newer libc++ (22 at least) writes ±inf or 0 into the value on result_out_of_range, where the standard keeps
// the value as it was, so the floating-point form always parses into a temporary first.
#pragma once

#include <charconv>
#include <system_error>
#include <type_traits>

#if defined(__APPLE__)
#include <Availability.h>
#endif

// Apple's libc++ has floating-point from_chars only from macOS 26; an app that runs on older macOS parses itself.
#if (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 200000) ||                                                     \
    (defined(__APPLE__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED < 260000)
#include <cerrno>
#include <clocale>
#include <cstdlib>
#include <string>
#if defined(__APPLE__)
#include <xlocale.h>
#else
#include <locale.h>
#endif
#define GS_FLOAT_FROM_CHARS_FALLBACK 1
#endif

namespace gs {

template <typename T>
    requires std::is_integral_v<T>
std::from_chars_result fromChars(const char* first, const char* last, T& value, int base = 10) noexcept
{
    return std::from_chars(first, last, value, base);
}

template <typename T>
    requires std::is_floating_point_v<T>
std::from_chars_result fromChars(const char* first, const char* last, T& value) noexcept
{
#if defined(GS_FLOAT_FROM_CHARS_FALLBACK)
    const char* digits = first;
    if (digits != last && *digits == '-') {
        ++digits;
    }
    if (digits == last) {
        return {first, std::errc::invalid_argument};
    }
    const char lead = *digits;
    const bool plausible = (lead >= '0' && lead <= '9') || lead == '.' || lead == 'i' || lead == 'I' || lead == 'n' ||
                           lead == 'N';
    if (!plausible) {
        return {first, std::errc::invalid_argument};
    }
    const char* stopAt = last;
    if (lead == '0' && digits + 1 != last && (digits[1] == 'x' || digits[1] == 'X')) {
        stopAt = digits + 1;
    }
    try {
        static const locale_t c = newlocale(LC_ALL_MASK, "C", nullptr);
        const std::string text(first, stopAt);
        char* end = nullptr;
        errno = 0;
        T parsed{};
        if constexpr (std::is_same_v<T, float>) {
            parsed = strtof_l(text.c_str(), &end, c);
        } else if constexpr (std::is_same_v<T, double>) {
            parsed = strtod_l(text.c_str(), &end, c);
        } else {
            parsed = strtold_l(text.c_str(), &end, c);
        }
        if (end == text.c_str()) {
            return {first, std::errc::invalid_argument};
        }
        const char* stop = first + (end - text.c_str());
        if (errno == ERANGE) {
            return {stop, std::errc::result_out_of_range};
        }
        value = parsed;
        return {stop, std::errc{}};
    } catch (...) {
        return {first, std::errc::not_enough_memory};
    }
#else
    T parsed{};
    const std::from_chars_result result = std::from_chars(first, last, parsed);
    if (result.ec == std::errc{}) {
        value = parsed;
    }
    return result;
#endif
}

}
