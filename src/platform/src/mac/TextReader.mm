// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Text recognition on macOS with the Vision framework (VNRecognizeTextRequest). Small pictures are enlarged
// first, as on Windows; boxes come back in the picture's own pixels, lines top to bottom.
#include "gs/platform/TextReader.h"

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>

#include <algorithm>
#include <cmath>
#include <optional>

namespace gs::platform {

namespace {

constexpr int kSmallSide = 300;
constexpr int kSmallLongSide = 1600;
constexpr double kMostEnlargement = 4.0;
constexpr int kMostSide = 8192;

std::string lowered(std::string_view text)
{
    std::string out(text);
    std::ranges::transform(out, out.begin(),
                           [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
    return out;
}

std::string utf8(NSString* text)
{
    const char* bytes = text.UTF8String;
    return bytes != nullptr ? std::string(bytes) : std::string{};
}

std::vector<std::string> supportedTags()
{
    @autoreleasepool {
        VNRecognizeTextRequest* request = [[VNRecognizeTextRequest alloc] init];
        request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
        NSError* error = nil;
        NSArray<NSString*>* languages = [request supportedRecognitionLanguagesAndReturnError:&error];
        std::vector<std::string> tags;
        for (NSString* language in languages) {
            tags.push_back(utf8(language));
        }
        return tags;
    }
}

std::optional<std::string> matched(const std::vector<std::string>& tags, std::string_view language)
{
    const std::string wanted = lowered(language);
    for (const std::string& tag : tags) {
        const std::string each = lowered(tag);
        if (each == wanted || each.starts_with(wanted + "-") || wanted.starts_with(each + "-")) {
            return tag;
        }
    }
    return std::nullopt;
}

std::optional<std::string> chosen(std::string_view language)
{
    const std::vector<std::string> tags = supportedTags();
    if (tags.empty()) {
        return std::nullopt;
    }
    if (!language.empty()) {
        return matched(tags, language);
    }
    @autoreleasepool {
        for (NSString* preferred in [NSLocale preferredLanguages]) {
            if (std::optional<std::string> tag = matched(tags, utf8(preferred))) {
                return tag;
            }
            const std::string base = utf8(preferred).substr(0, utf8(preferred).find('-'));
            if (std::optional<std::string> tag = matched(tags, base)) {
                return tag;
            }
        }
    }
    return tags.front();
}

std::vector<std::uint8_t> scaled(const std::uint8_t* grey, int width, int height, std::ptrdiff_t stride, int outWidth,
                                 int outHeight)
{
    std::vector<std::uint8_t> out(static_cast<std::size_t>(outWidth) * static_cast<std::size_t>(outHeight));
    const double stepX = static_cast<double>(width) / outWidth;
    const double stepY = static_cast<double>(height) / outHeight;
    for (int y = 0; y < outHeight; ++y) {
        const double fromY = std::clamp((y + 0.5) * stepY - 0.5, 0.0, height - 1.0);
        const int y0 = static_cast<int>(fromY);
        const int y1 = std::min(y0 + 1, height - 1);
        const double partY = fromY - y0;
        const std::uint8_t* upper = grey + static_cast<std::ptrdiff_t>(y0) * stride;
        const std::uint8_t* lower = grey + static_cast<std::ptrdiff_t>(y1) * stride;
        std::uint8_t* row = out.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(outWidth);
        for (int x = 0; x < outWidth; ++x) {
            const double fromX = std::clamp((x + 0.5) * stepX - 0.5, 0.0, width - 1.0);
            const int x0 = static_cast<int>(fromX);
            const int x1 = std::min(x0 + 1, width - 1);
            const double partX = fromX - x0;
            const double top = upper[x0] + (upper[x1] - upper[x0]) * partX;
            const double bottom = lower[x0] + (lower[x1] - lower[x0]) * partX;
            row[x] = static_cast<std::uint8_t>(std::clamp(std::lround(top + (bottom - top) * partY), 0L, 255L));
        }
    }
    return out;
}

char32_t firstCodePoint(std::string_view text) noexcept
{
    if (text.empty()) {
        return 0;
    }
    const auto lead = static_cast<unsigned char>(text[0]);
    const std::size_t length = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
    char32_t point = length == 1 ? lead : length == 2 ? lead & 0x1Fu : length == 3 ? lead & 0x0Fu : lead & 0x07u;
    for (std::size_t at = 1; at < length && at < text.size(); ++at) {
        point = (point << 6) | (static_cast<unsigned char>(text[at]) & 0x3Fu);
    }
    return point;
}

char32_t lastCodePoint(std::string_view text) noexcept
{
    std::size_t start = text.size();
    while (start > 0) {
        --start;
        if ((static_cast<unsigned char>(text[start]) & 0xC0u) != 0x80u) {
            break;
        }
    }
    return firstCodePoint(text.substr(start));
}

TextBox boxOf(CGRect normalized, int width, int height)
{
    return {normalized.origin.x * width, (1.0 - normalized.origin.y - normalized.size.height) * height,
            normalized.size.width * width, normalized.size.height * height};
}

TextBox joined(const TextBox& one, const TextBox& other)
{
    const double left = std::min(one.x, other.x);
    const double top = std::min(one.y, other.y);
    const double right = std::max(one.x + one.width, other.x + other.width);
    const double bottom = std::max(one.y + one.height, other.y + other.height);
    return {left, top, right - left, bottom - top};
}

Expected<TextResult, TextProblem> recognized(const std::uint8_t* grey, int width, int height, std::ptrdiff_t stride,
                                             std::string_view language)
{
    const std::optional<std::string> tag = chosen(language);
    if (!tag) {
        return makeUnexpected(TextProblem::NoLanguage);
    }
    TextResult result;
    result.language = *tag;
    if (grey == nullptr || width <= 0 || height <= 0) {
        return result;
    }
    const double wanted = std::max(static_cast<double>(kSmallSide) / std::min(width, height),
                                   static_cast<double>(kSmallLongSide) / std::max(width, height));
    const double scale = std::min(std::clamp(wanted, 1.0, kMostEnlargement),
                                  static_cast<double>(kMostSide) / std::max(width, height));
    const int outWidth = std::max(1, static_cast<int>(std::lround(width * scale)));
    const int outHeight = std::max(1, static_cast<int>(std::lround(height * scale)));
    std::vector<std::uint8_t> pixels = scaled(grey, width, height, stride, outWidth, outHeight);

    @autoreleasepool {
        CGColorSpaceRef space = CGColorSpaceCreateDeviceGray();
        CGDataProviderRef provider = CGDataProviderCreateWithData(nullptr, pixels.data(), pixels.size(), nullptr);
        CGImageRef image = CGImageCreate(static_cast<std::size_t>(outWidth), static_cast<std::size_t>(outHeight), 8, 8,
                                         static_cast<std::size_t>(outWidth), space, kCGImageAlphaNone, provider,
                                         nullptr, false, kCGRenderingIntentDefault);
        CGDataProviderRelease(provider);
        CGColorSpaceRelease(space);
        if (image == nullptr) {
            return makeUnexpected(TextProblem::Failed);
        }
        VNRecognizeTextRequest* request = [[VNRecognizeTextRequest alloc] init];
        request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
        request.usesLanguageCorrection = YES;
        request.recognitionLanguages = @[ [NSString stringWithUTF8String:tag->c_str()] ];
        VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithCGImage:image options:@{}];
        NSError* error = nil;
        const BOOL performed = [handler performRequests:@[ request ] error:&error];
        CGImageRelease(image);
        if (!performed) {
            return makeUnexpected(TextProblem::Failed);
        }
        for (VNRecognizedTextObservation* observation in request.results) {
            VNRecognizedText* best = [[observation topCandidates:1] firstObject];
            if (best == nil || best.string.length == 0) {
                continue;
            }
            NSString* whole = best.string;
            __block TextLine made;
            const CGRect lineBox = observation.boundingBox;
            [whole enumerateSubstringsInRange:NSMakeRange(0, whole.length)
                                      options:NSStringEnumerationByWords
                                   usingBlock:^(NSString* word, NSRange range, NSRange, BOOL*) {
                                       std::string piece = utf8(word);
                                       if (piece.empty()) {
                                           return;
                                       }
                                       NSError* boxError = nil;
                                       VNRectangleObservation* place = [best boundingBoxForRange:range error:&boxError];
                                       const TextBox box = boxOf(place != nil ? place.boundingBox : lineBox, width, height);
                                       if (made.words.empty()) {
                                           made.box = box;
                                       } else {
                                           if (joinsWithSpace(lastCodePoint(made.text), firstCodePoint(piece))) {
                                               made.text += ' ';
                                           }
                                           made.box = joined(made.box, box);
                                       }
                                       made.text += piece;
                                       made.words.push_back({std::move(piece), box});
                                   }];
            if (!made.words.empty()) {
                result.lines.push_back(std::move(made));
            }
        }
    }
    std::ranges::stable_sort(result.lines, [](const TextLine& one, const TextLine& other) {
        return one.box.y + one.box.height / 2 < other.box.y + other.box.height / 2;
    });
    return result;
}

}

Expected<TextResult, TextProblem> readText(const std::uint8_t* grey, int width, int height, std::ptrdiff_t stride,
                                           std::string_view language)
{
    try {
        return recognized(grey, width, height, stride, language);
    } catch (...) {
        return makeUnexpected(TextProblem::Failed);
    }
}

std::vector<std::string> textLanguages()
{
    try {
        return supportedTags();
    } catch (...) {
        return {};
    }
}

TextLanguages::TextLanguages(Done done)
    : m_thread([done = std::move(done)] {
        std::vector<std::string> tags = textLanguages();
        if (done) {
            done(std::move(tags));
        }
    })
{
}

TextLanguages::~TextLanguages() = default;

}
