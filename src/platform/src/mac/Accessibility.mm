// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The system's motion and transparency settings on macOS: Accessibility > Display, Reduce motion and Reduce
// transparency.
#include "gs/platform/Accessibility.h"

#import <AppKit/AppKit.h>

namespace gs::platform {

bool systemAnimationsOn() noexcept
{
    @autoreleasepool {
        return ![[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
    }
}

bool systemTransparencyOn() noexcept
{
    @autoreleasepool {
        return ![[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceTransparency];
    }
}

}
