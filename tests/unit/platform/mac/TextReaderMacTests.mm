// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Text recognition with the Vision framework: pictures drawn with Core Text, read back.
#include "gs/platform/TextReader.h"

#include <doctest/doctest.h>

#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <string>
#include <vector>

using gs::platform::readText;
using gs::platform::TextProblem;

namespace {

struct Picture {
    std::vector<std::uint8_t> grey;
    int width = 0;
    int height = 0;
};

Picture drawn(NSString* text, int width, int height, double left, double top, double size, NSString* face)
{
    Picture picture{std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height, 255), width, height};
    CGColorSpaceRef space = CGColorSpaceCreateDeviceGray();
    CGContextRef context = CGBitmapContextCreate(picture.grey.data(), width, height, 8, width, space, kCGImageAlphaNone);
    CGColorSpaceRelease(space);
    CTFontRef font = CTFontCreateWithName((__bridge CFStringRef)face, size, nullptr);
    NSDictionary* attributes = @{
        (__bridge NSString*)kCTFontAttributeName : (__bridge id)font,
        (__bridge NSString*)kCTForegroundColorFromContextAttributeName : @YES,
    };
    NSAttributedString* string = [[NSAttributedString alloc] initWithString:text attributes:attributes];
    CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)string);
    CGContextSetGrayFillColor(context, 0.0, 1.0);
    CGContextSetTextPosition(context, left, height - top - size);
    CTLineDraw(line, context);
    CFRelease(line);
    CFRelease(font);
    CGContextRelease(context);
    return picture;
}

std::string squeezed(std::string_view text)
{
    std::string out;
    for (const char c : text) {
        if (c != ' ') {
            out += c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
        }
    }
    return out;
}

}

TEST_CASE("macOS reads an English line with its place, and each word with its own")
{
    const Picture picture = drawn(@"Ready to play", 480, 120, 40, 30, 36, @"Helvetica");
    const auto read = readText(picture.grey.data(), picture.width, picture.height, picture.width, "en");
    REQUIRE(read.has_value());
    CHECK(read->language.starts_with("en"));
    REQUIRE(read->lines.size() == 1);
    const auto& line = read->lines.front();
    CHECK(squeezed(line.text) == "readytoplay");
    CHECK(line.words.size() == 3);
    CHECK(line.box.x == doctest::Approx(40).epsilon(0.5));
    CHECK(line.box.y > 10);
    CHECK(line.box.y < 50);
    CHECK(line.box.width > 150);
    for (std::size_t i = 1; i < line.words.size(); ++i) {
        CHECK(line.words[i].box.x > line.words[i - 1].box.x);
    }
}

TEST_CASE("macOS joins Chinese words without spaces")
{
    const Picture picture = drawn(@"电池电量低", 480, 120, 40, 30, 40, @"PingFang SC");
    const auto read = readText(picture.grey.data(), picture.width, picture.height, picture.width, "zh-Hans");
    REQUIRE(read.has_value());
    REQUIRE(read->lines.size() == 1);
    CHECK(read->lines.front().text.find(' ') == std::string::npos);
    CHECK(read->lines.front().text == "电池电量低");
}

TEST_CASE("macOS names a language it cannot read and picks one by default")
{
    const Picture picture = drawn(@"Ready", 200, 80, 10, 10, 28, @"Helvetica");
    const auto missing = readText(picture.grey.data(), picture.width, picture.height, picture.width, "xx");
    REQUIRE_FALSE(missing.has_value());
    CHECK(missing.error() == TextProblem::NoLanguage);
    const std::vector<std::string> languages = gs::platform::textLanguages();
    REQUIRE_FALSE(languages.empty());
    const auto chosen = readText(picture.grey.data(), picture.width, picture.height, picture.width, "");
    REQUIRE(chosen.has_value());
    CHECK(std::ranges::find(languages, chosen->language) != languages.end());
    const auto empty = readText(nullptr, 0, 0, 0, "en");
    REQUIRE(empty.has_value());
    CHECK(empty->lines.empty());
}
