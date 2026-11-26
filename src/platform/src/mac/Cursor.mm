// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The window's area and the cursor on macOS, in global display coordinates (points, top left origin, as Quartz
// gives them). Pinning the cursor puts it on one point of the window and detaches it from the mouse, so it stays
// there while the mouse keeps sending its movements (Windows clips the cursor to a 1x1 rectangle instead).
#include "gs/platform/Cursor.h"

#import <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>

#include <algorithm>
#include <atomic>
#include <cmath>

namespace gs::platform {

namespace {

std::atomic<bool> g_pinned{false};

}

std::optional<ScreenArea> clientArea(std::uintptr_t window) noexcept
{
    @autoreleasepool {
        auto* view = (__bridge NSView*)reinterpret_cast<void*>(window);
        NSWindow* host = view != nil ? [view window] : nil;
        NSScreen* primary = [[NSScreen screens] firstObject];
        if (host == nil || primary == nil) {
            return std::nullopt;
        }
        const NSRect inWindow = [view convertRect:[view bounds] toView:nil];
        const NSRect onScreen = [host convertRectToScreen:inWindow];
        const CGFloat top = NSMaxY([primary frame]) - NSMaxY(onScreen);
        return ScreenArea{static_cast<int>(std::lround(NSMinX(onScreen))), static_cast<int>(std::lround(top)),
                          static_cast<int>(std::lround(NSWidth(onScreen))),
                          static_cast<int>(std::lround(NSHeight(onScreen)))};
    }
}

std::optional<ScreenPoint> cursorPosition() noexcept
{
    CGEventRef event = CGEventCreate(nullptr);
    if (event == nullptr) {
        return std::nullopt;
    }
    const CGPoint at = CGEventGetLocation(event);
    CFRelease(event);
    return ScreenPoint{static_cast<int>(std::lround(at.x)), static_cast<int>(std::lround(at.y))};
}

std::optional<ScreenArea> pinnedArea(const ScreenArea& client, double x, double y) noexcept
{
    if (client.width <= 0 || client.height <= 0 || !std::isfinite(x) || !std::isfinite(y)) {
        return std::nullopt;
    }
    const auto at = [](int origin, int size, double fraction) {
        const long offset = std::lround(size * std::clamp(fraction, 0.0, 1.0));
        return origin + static_cast<int>(std::min<long>(offset, size - 1));
    };
    return ScreenArea{at(client.x, client.width, x), at(client.y, client.height, y), 1, 1};
}

bool pinCursorInClientArea(std::uintptr_t window, double x, double y) noexcept
{
    const std::optional<ScreenArea> client = clientArea(window);
    const std::optional<ScreenArea> pin = client ? pinnedArea(*client, x, y) : std::nullopt;
    if (!pin) {
        return false;
    }
    if (CGWarpMouseCursorPosition(CGPointMake(pin->x, pin->y)) != kCGErrorSuccess) {
        return false;
    }
    g_pinned = true;
    return CGAssociateMouseAndMouseCursorPosition(false) == kCGErrorSuccess;
}

void releaseCursorClip() noexcept
{
    if (g_pinned.exchange(false)) {
        CGAssociateMouseAndMouseCursorPosition(true);
    }
}

// Windows asks the caption about hit tests in its messages; a macOS window has its own title bar.
CaptionHover captionHoverOf(const void*, std::uintptr_t) noexcept
{
    return CaptionHover::Unrelated;
}

bool pointerOnCaption(std::uintptr_t) noexcept
{
    return false;
}

}
