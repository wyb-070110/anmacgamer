// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Pictures on macOS: ImageIO reads them, Core Graphics draws them into non-premultiplied RGBA (as WIC gives
// on Windows) and scales them with its best interpolation.
#include "Pictures.h"

#include <gs/vision/Matcher.h>

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <ImageIO/ImageIO.h>

#include <cstdint>
#include <string>

namespace gs::record {

namespace {

constexpr std::size_t kBytesPerPixel = 4;

template <typename T>
class Owned
{
public:
    explicit Owned(T ref = nullptr) noexcept : m_ref(ref) {}
    ~Owned()
    {
        if (m_ref != nullptr) {
            CFRelease(m_ref);
        }
    }
    Owned(const Owned&) = delete;
    Owned& operator=(const Owned&) = delete;

    [[nodiscard]] T get() const noexcept { return m_ref; }
    [[nodiscard]] explicit operator bool() const noexcept { return m_ref != nullptr; }

private:
    T m_ref;
};

void unpremultiply(std::vector<std::uint8_t>& rgba) noexcept
{
    for (std::size_t at = 0; at + 3 < rgba.size(); at += kBytesPerPixel) {
        const unsigned alpha = rgba[at + 3];
        if (alpha == 0 || alpha == 255) {
            continue;
        }
        for (std::size_t channel = 0; channel < 3; ++channel) {
            const unsigned value = (rgba[at + channel] * 255U + alpha / 2) / alpha;
            rgba[at + channel] = static_cast<std::uint8_t>(value > 255U ? 255U : value);
        }
    }
}

bool drawn(CGImageRef image, int width, int height, std::vector<std::uint8_t>& rgba)
{
    rgba.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kBytesPerPixel, 0);
    CGColorSpaceRef imageSpace = CGImageGetColorSpace(image);
    const bool rgb = imageSpace != nullptr && CGColorSpaceGetModel(imageSpace) == kCGColorSpaceModelRGB;
    const Owned<CGColorSpaceRef> srgb(rgb ? nullptr : CGColorSpaceCreateWithName(kCGColorSpaceSRGB));
    const Owned<CGContextRef> context(CGBitmapContextCreate(
        rgba.data(), static_cast<std::size_t>(width), static_cast<std::size_t>(height), 8,
        static_cast<std::size_t>(width) * kBytesPerPixel, rgb ? imageSpace : srgb.get(),
        static_cast<std::uint32_t>(kCGImageAlphaPremultipliedLast) | static_cast<std::uint32_t>(kCGBitmapByteOrder32Big)));
    if (!context) {
        return false;
    }
    CGContextSetBlendMode(context.get(), kCGBlendModeCopy);
    CGContextSetInterpolationQuality(context.get(), kCGInterpolationHigh);
    CGContextDrawImage(context.get(), CGRectMake(0, 0, width, height), image);
    unpremultiply(rgba);
    return true;
}

bool hasAlpha(CGImageRef image) noexcept
{
    switch (CGImageGetAlphaInfo(image)) {
    case kCGImageAlphaNone:
    case kCGImageAlphaNoneSkipFirst:
    case kCGImageAlphaNoneSkipLast:
        return false;
    default:
        return true;
    }
}

}

struct PictureReader::State {};

PictureReader::PictureReader() : m_state(std::make_unique<State>()) {}

PictureReader::~PictureReader() = default;

PictureProblem PictureReader::read(const std::filesystem::path& file, Picture& picture)
{
    picture = {};
    std::error_code error;
    if (!std::filesystem::is_regular_file(file, error)) {
        return PictureProblem::Missing;
    }
    const std::string path = file.string();
    const Owned<CFURLRef> url(CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(path.data()), static_cast<CFIndex>(path.size()), false));
    const Owned<CGImageSourceRef> source(url ? CGImageSourceCreateWithURL(url.get(), nullptr) : nullptr);
    const Owned<CGImageRef> image(source ? CGImageSourceCreateImageAtIndex(source.get(), 0, nullptr) : nullptr);
    if (!image) {
        return PictureProblem::Unreadable;
    }
    const auto width = static_cast<std::int64_t>(CGImageGetWidth(image.get()));
    const auto height = static_cast<std::int64_t>(CGImageGetHeight(image.get()));
    if (width <= 0 || height <= 0) {
        return PictureProblem::Unreadable;
    }
    if (width > vision::kMaxTemplateSide || height > vision::kMaxTemplateSide ||
        width * height > vision::kMaxTemplatePixels) {
        return PictureProblem::TooLarge;
    }
    picture.alpha = hasAlpha(image.get());
    if (!drawn(image.get(), static_cast<int>(width), static_cast<int>(height), picture.rgba)) {
        picture = {};
        return PictureProblem::Unreadable;
    }
    picture.width = static_cast<int>(width);
    picture.height = static_cast<int>(height);
    return PictureProblem::None;
}

Picture PictureReader::scaled(const Picture& picture, int width, int height)
{
    if (picture.empty() || width <= 0 || height <= 0 || (width == picture.width && height == picture.height)) {
        return picture;
    }
    const Owned<CGColorSpaceRef> space(CGColorSpaceCreateWithName(kCGColorSpaceSRGB));
    const Owned<CFDataRef> bytes(CFDataCreate(kCFAllocatorDefault, picture.rgba.data(),
                                              static_cast<CFIndex>(picture.rgba.size())));
    const Owned<CGDataProviderRef> provider(bytes ? CGDataProviderCreateWithCFData(bytes.get()) : nullptr);
    const Owned<CGImageRef> image(
        provider ? CGImageCreate(static_cast<std::size_t>(picture.width), static_cast<std::size_t>(picture.height), 8,
                                 32, static_cast<std::size_t>(picture.width) * kBytesPerPixel, space.get(),
                                 static_cast<std::uint32_t>(kCGImageAlphaLast) |
                                     static_cast<std::uint32_t>(kCGBitmapByteOrder32Big),
                                 provider.get(), nullptr, false, kCGRenderingIntentDefault)
                 : nullptr);
    Picture result{.width = width, .height = height, .alpha = picture.alpha, .rgba = {}};
    if (!image || !drawn(image.get(), width, height, result.rgba)) {
        return picture;
    }
    return result;
}

}
