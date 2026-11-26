// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Whether a window is in front, the mouse buttons and keys held, and changes of the front application, on macOS.
// A window is the NSView of a QWindow (its winId()).
#include "gs/platform/Foreground.h"

#include "KeyCodes.h"

#import <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>

namespace gs::platform {

bool isInFront(std::uintptr_t window) noexcept
{
    @autoreleasepool {
        auto* view = (__bridge NSView*)reinterpret_cast<void*>(window);
        return view != nil && [NSApp isActive] && [[view window] isKeyWindow];
    }
}

std::uint8_t pressedMouseButtons() noexcept
{
    const NSUInteger held = [NSEvent pressedMouseButtons];
    std::uint8_t buttons = 0;
    buttons |= (held & (1U << 0)) != 0 ? mouse_button::kLeft : 0;
    buttons |= (held & (1U << 1)) != 0 ? mouse_button::kRight : 0;
    buttons |= (held & (1U << 2)) != 0 ? mouse_button::kMiddle : 0;
    buttons |= (held & (1U << 3)) != 0 ? mouse_button::kX1 : 0;
    buttons |= (held & (1U << 4)) != 0 ? mouse_button::kX2 : 0;
    return buttons;
}

bool scanCodeHeld(std::uint16_t scanCode) noexcept
{
    const std::optional<std::uint16_t> key = mac::keyOfScanCode(scanCode);
    return key && CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, static_cast<CGKeyCode>(*key));
}

struct ForegroundWatcher::Impl {
    Changed changed;
    std::vector<id> observers;
};

ForegroundWatcher::ForegroundWatcher(Changed changed) : m_impl(std::make_unique<Impl>())
{
    m_impl->changed = std::move(changed);
    @autoreleasepool {
        Impl* impl = m_impl.get();
        void (^notify)(NSNotification*) = ^(NSNotification*) {
            if (impl->changed) {
                impl->changed();
            }
        };
        NSNotificationCenter* workspace = [[NSWorkspace sharedWorkspace] notificationCenter];
        impl->observers.push_back([workspace addObserverForName:NSWorkspaceDidActivateApplicationNotification
                                                         object:nil
                                                          queue:[NSOperationQueue mainQueue]
                                                     usingBlock:notify]);
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        for (NSNotificationName name : {NSApplicationDidBecomeActiveNotification,
                                        NSApplicationDidResignActiveNotification,
                                        NSWindowDidBecomeKeyNotification, NSWindowDidResignKeyNotification}) {
            impl->observers.push_back([center addObserverForName:name
                                                          object:nil
                                                           queue:[NSOperationQueue mainQueue]
                                                      usingBlock:notify]);
        }
    }
}

ForegroundWatcher::~ForegroundWatcher()
{
    @autoreleasepool {
        NSNotificationCenter* workspace = [[NSWorkspace sharedWorkspace] notificationCenter];
        for (id observer : m_impl->observers) {
            [workspace removeObserver:observer];
            [[NSNotificationCenter defaultCenter] removeObserver:observer];
        }
    }
}

bool ForegroundWatcher::started() const noexcept
{
    return !m_impl->observers.empty();
}

}
