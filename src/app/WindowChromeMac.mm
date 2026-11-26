// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The window frame on macOS: the system's title bar, made transparent with the content drawn under it, so the
// window keeps the traffic lights and macOS moves it by its top strip and zooms it on a double click there (the
// System Settings choice). QWindowKit's frameless window is not used on macOS: it makes the window unmovable and
// the transparent title bar then keeps the mouse from Qt.
#include "WindowChromeMac.h"

#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#include <cmath>

#include <QWindow>

// Stands in front of Qt's window delegate: answers windowWillResize:toSize: so the picture keeps its proportions,
// and hands every other message to Qt's delegate.
@interface GSAspectDelegate : NSObject <NSWindowDelegate>
@property(strong) id<NSWindowDelegate> inner;
@property double aspect;
@property NSSize chrome;
@end

@implementation GSAspectDelegate

- (BOOL)respondsToSelector:(SEL)selector
{
    return [super respondsToSelector:selector] || [self.inner respondsToSelector:selector];
}

- (id)forwardingTargetForSelector:(SEL)selector
{
    return [self.inner respondsToSelector:selector] ? self.inner : nil;
}

- (NSSize)windowWillResize:(NSWindow*)window toSize:(NSSize)frameSize
{
    NSSize proposed = frameSize;
    if ([self.inner respondsToSelector:@selector(windowWillResize:toSize:)]) {
        proposed = [self.inner windowWillResize:window toSize:frameSize];
    }
    if (!(self.aspect > 0) || (window.styleMask & NSWindowStyleMaskFullScreen) != 0) {
        return proposed;
    }
    const NSSize content = [window contentRectForFrameRect:NSMakeRect(0, 0, proposed.width, proposed.height)].size;
    const NSSize current = [window contentRectForFrameRect:window.frame].size;
    CGFloat width = content.width - self.chrome.width;
    CGFloat height = content.height - self.chrome.height;
    if (width <= 0 || height <= 0) {
        return proposed;
    }
    // The edge that moved more leads; the other side follows the picture's proportions.
    if (std::abs(content.width - current.width) >= std::abs(content.height - current.height)) {
        height = width / self.aspect;
    } else {
        width = height * self.aspect;
    }
    const NSRect fitted = NSMakeRect(0, 0, std::round(width + self.chrome.width), std::round(height + self.chrome.height));
    return [window frameRectForContentRect:fitted].size;
}

@end

namespace gs::app {

namespace {

NSWindow* nativeWindow(QWindow* window)
{
    if (window == nullptr) {
        return nil;
    }
    auto* view = (__bridge NSView*)reinterpret_cast<void*>(window->winId());
    return view != nil ? [view window] : nil;
}

}

bool styleMacWindow(QWindow* window)
{
    NSWindow* host = nativeWindow(window);
    if (host == nil) {
        return false;
    }
    host.styleMask |= NSWindowStyleMaskFullSizeContentView;
    host.titlebarAppearsTransparent = YES;
    host.titleVisibility = NSWindowTitleHidden;
    host.movable = YES;
    return true;
}

void setMacWindowOnTop(QWindow* window, bool on)
{
    NSWindow* host = nativeWindow(window);
    if (host == nil) {
        return;
    }
    constexpr NSWindowCollectionBehavior kEverywhere =
        NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary;
    host.level = on ? NSFloatingWindowLevel : NSNormalWindowLevel;
    host.hidesOnDeactivate = NO;
    host.collectionBehavior = on ? (host.collectionBehavior | kEverywhere) : (host.collectionBehavior & ~kEverywhere);
}

void setMacWindowDark(QWindow* window, bool dark)
{
    NSWindow* host = nativeWindow(window);
    if (host != nil) {
        host.appearance = [NSAppearance appearanceNamed:dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
    }
}

void setMacWindowGlass(QWindow* window, bool on)
{
    NSString* const kGlassId = @"io.github.nuoran.gamescrcpy.glass";
    NSWindow* host = nativeWindow(window);
    if (host == nil || host.contentView == nil) {
        return;
    }
    NSView* frame = host.contentView.superview;
    NSVisualEffectView* glass = nil;
    for (NSView* view in frame.subviews) {
        if ([view isKindOfClass:[NSVisualEffectView class]] && [view.identifier isEqualToString:kGlassId]) {
            glass = (NSVisualEffectView*)view;
        }
    }
    if (!on) {
        [glass removeFromSuperview];
        host.opaque = YES;
        host.backgroundColor = NSColor.windowBackgroundColor;
        return;
    }
    host.opaque = NO;
    host.backgroundColor = NSColor.clearColor;
    if (glass == nil) {
        glass = [[NSVisualEffectView alloc] initWithFrame:frame.bounds];
        glass.identifier = kGlassId;
        glass.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        glass.blendingMode = NSVisualEffectBlendingModeBehindWindow;
        glass.material = NSVisualEffectMaterialSidebar;
        glass.state = NSVisualEffectStateFollowsWindowActiveState;
        [frame addSubview:glass positioned:NSWindowBelow relativeTo:host.contentView];
    }
    host.contentView.wantsLayer = YES;
    host.contentView.layer.opaque = NO;
}

void setMacWindowAspect(QWindow* window, double aspect, int chromeWidth, int chromeHeight)
{
    static char kAspectKey = 0;
    NSWindow* host = nativeWindow(window);
    if (host == nil) {
        return;
    }
    GSAspectDelegate* proxy = objc_getAssociatedObject(host, &kAspectKey);
    if (proxy == nil) {
        if (!(aspect > 0)) {
            return;
        }
        proxy = [[GSAspectDelegate alloc] init];
        proxy.inner = host.delegate;
        objc_setAssociatedObject(host, &kAspectKey, proxy, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        host.delegate = proxy;
    }
    proxy.aspect = aspect;
    proxy.chrome = NSMakeSize(chromeWidth, chromeHeight);
}

bool startMacWindowDrag(QWindow* window)
{
    NSWindow* host = nativeWindow(window);
    NSEvent* current = [NSApp currentEvent];
    if (host == nil || current == nil) {
        return false;
    }
    const NSEventType type = current.type;
    if (type != NSEventTypeLeftMouseDown && type != NSEventTypeLeftMouseDragged) {
        return false;
    }
    [host performWindowDragWithEvent:current];
    return true;
}

void performMacTitleBarDoubleClick(QWindow* window)
{
    NSWindow* host = nativeWindow(window);
    if (host == nil || (host.styleMask & NSWindowStyleMaskFullScreen) != 0) {
        return;
    }
    NSString* action = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleActionOnDoubleClick"];
    if ([action isEqualToString:@"None"]) {
        return;
    }
    if ([action isEqualToString:@"Minimize"]) {
        if ((host.styleMask & NSWindowStyleMaskMiniaturizable) != 0) {
            [host performMiniaturize:nil];
        }
        return;
    }
    if ((host.styleMask & NSWindowStyleMaskResizable) != 0) {
        [host zoom:nil];
    }
}

}
