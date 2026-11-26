// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
#include "gs/base/FromChars.h"

#include <doctest/doctest.h>

#include <cmath>
#include <string_view>

namespace {

template <typename T>
std::from_chars_result parse(std::string_view text, T& value)
{
    return gs::fromChars(text.data(), text.data() + text.size(), value);
}

}

TEST_CASE("fromChars reads decimals as from_chars does, in every locale")
{
    double value = 0.0;
    auto result = parse("2.5", value);
    CHECK(result.ec == std::errc{});
    CHECK(value == 2.5);
    CHECK(parse("-1e3", value).ec == std::errc{});
    CHECK(value == -1000.0);
    result = parse("12.5ms", value);
    CHECK(result.ec == std::errc{});
    CHECK(value == 12.5);
    CHECK(std::string_view(result.ptr) == "ms");
}

TEST_CASE("fromChars refuses what from_chars refuses and keeps the value")
{
    double value = 7.0;
    CHECK(parse("", value).ec == std::errc::invalid_argument);
    CHECK(parse(" 1", value).ec == std::errc::invalid_argument);
    CHECK(parse("+1", value).ec == std::errc::invalid_argument);
    CHECK(parse("abc", value).ec == std::errc::invalid_argument);
    CHECK(value == 7.0);
    CHECK(parse("1e999", value).ec == std::errc::result_out_of_range);
    CHECK(value == 7.0);
    const auto hex = parse("0x1A", value);
    CHECK(hex.ec == std::errc{});
    CHECK(value == 0.0);
    CHECK(std::string_view(hex.ptr) == "x1A");
}

TEST_CASE("fromChars passes integers through with their base")
{
    int value = 0;
    CHECK(parse("42", value).ec == std::errc{});
    CHECK(value == 42);
    unsigned hex = 0;
    const std::string_view text = "ff";
    CHECK(gs::fromChars(text.data(), text.data() + text.size(), hex, 16).ec == std::errc{});
    CHECK(hex == 255U);
    float single = 0.0F;
    CHECK(parse("0.25", single).ec == std::errc{});
    CHECK(single == 0.25F);
}
