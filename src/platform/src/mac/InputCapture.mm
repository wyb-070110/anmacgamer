// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Keyboard, mouse and gamepads for the key map, on macOS. A thread of its own runs a CFRunLoop: tasks posted to
// the capture run there, gamepads are polled there with SDL (as on Windows), and the handler is woken when it asks
// (nextWakeup).
//
// Keys and the mouse come from the program's own events: while a window is the capture target, a local NSEvent
// monitor on the main thread reads them as they arrive (before Qt) and hands them to the input thread. This needs
// no "Input Monitoring" permission. Keys are named by their PC scan codes (KeyCodes.h), the mouse by Raw Input's
// button flags and wheel steps, so the rest of the program reads them as on Windows. Mouse movement is the pointer's
// (with the system's acceleration); while the cursor is locked (Cursor.mm) it still arrives as relative movement.
#include "gs/platform/InputCapture.h"

#include "gs/base/Clock.h"
#include "gs/platform/Thread.h"

#include "KeyCodes.h"

#import <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>

#if defined(GS_HAVE_SDL3)
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#endif

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

namespace gs::platform {

namespace {

constexpr Microseconds kProbeInterval{500'000};
constexpr double kNever = 3600.0 * 24 * 365;

// Raw Input's mouse button flags (gs::input::rawmouse), which RawMouse carries.
constexpr std::uint16_t kLeftDown = 0x0001;
constexpr std::uint16_t kLeftUp = 0x0002;
constexpr std::uint16_t kRightDown = 0x0004;
constexpr std::uint16_t kRightUp = 0x0008;
constexpr std::uint16_t kMiddleDown = 0x0010;
constexpr std::uint16_t kMiddleUp = 0x0020;
constexpr std::uint16_t kX1Down = 0x0040;
constexpr std::uint16_t kX1Up = 0x0080;
constexpr std::uint16_t kX2Down = 0x0100;
constexpr std::uint16_t kX2Up = 0x0200;
constexpr std::uint16_t kWheel = 0x0400;
constexpr std::uint16_t kHorizontalWheel = 0x0800;
constexpr int kWheelStep = 120;
constexpr int kMostNotches = 8;
// A trackpad scrolls in points: this many make one wheel step.
constexpr double kPointsPerStep = 40.0;

// The modifier keys arrive as flagsChanged; whether one went down is in its own device bit (IOLLEvent.h's
// NX_DEVICE*KEYMASK), which tells the left key from the right one.
std::optional<bool> modifierDown(std::uint16_t key, NSEventModifierFlags flags)
{
    std::uint64_t bit = 0;
    switch (key) {
    case 0x38: bit = 0x00000002; break; // left shift
    case 0x3C: bit = 0x00000004; break; // right shift
    case 0x3B: bit = 0x00000001; break; // left control
    case 0x3E: bit = 0x00002000; break; // right control
    case 0x3A: bit = 0x00000020; break; // left option
    case 0x3D: bit = 0x00000040; break; // right option
    case 0x37: bit = 0x00000008; break; // left command
    case 0x36: bit = 0x00000010; break; // right command
    default: return std::nullopt;
    }
    return (flags & bit) != 0;
}

#if defined(GS_HAVE_SDL3)
constexpr int kAxisMax = 32767;
constexpr int kTriggerMax = 255;

PadKind kindOf(SDL_GamepadType type) noexcept
{
    switch (type) {
    case SDL_GAMEPAD_TYPE_XBOX360:
    case SDL_GAMEPAD_TYPE_XBOXONE:
        return PadKind::Xbox;
    case SDL_GAMEPAD_TYPE_PS4:
        return PadKind::PlayStation4;
    case SDL_GAMEPAD_TYPE_PS5:
        return PadKind::PlayStation5;
    default:
        return PadKind::Other;
    }
}

PadState padStateOf(SDL_Gamepad* pad)
{
    constexpr std::pair<SDL_GamepadButton, std::uint16_t> kButtons[] = {
        {SDL_GAMEPAD_BUTTON_DPAD_UP, 0x0001},       {SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0x0002},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0x0004},     {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0x0008},
        {SDL_GAMEPAD_BUTTON_START, 0x0010},         {SDL_GAMEPAD_BUTTON_BACK, 0x0020},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, 0x0040},    {SDL_GAMEPAD_BUTTON_RIGHT_STICK, 0x0080},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0x0100}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0x0200},
        {SDL_GAMEPAD_BUTTON_SOUTH, 0x1000},         {SDL_GAMEPAD_BUTTON_EAST, 0x2000},
        {SDL_GAMEPAD_BUTTON_WEST, 0x4000},          {SDL_GAMEPAD_BUTTON_NORTH, 0x8000},
    };
    std::uint16_t buttons = 0;
    for (const auto& [button, bit] : kButtons) {
        if (SDL_GetGamepadButton(pad, button)) {
            buttons = static_cast<std::uint16_t>(buttons | bit);
        }
    }
    const auto trigger = [pad](SDL_GamepadAxis axis) {
        return static_cast<std::uint8_t>(std::clamp<int>(SDL_GetGamepadAxis(pad, axis), 0, kAxisMax) * kTriggerMax /
                                         kAxisMax);
    };
    const auto upward = [pad](SDL_GamepadAxis axis) {
        return static_cast<std::int16_t>(
            std::clamp(-static_cast<int>(SDL_GetGamepadAxis(pad, axis)), -kAxisMax - 1, kAxisMax));
    };
    constexpr std::pair<SDL_GamepadButton, std::uint8_t> kExtras[] = {
        {SDL_GAMEPAD_BUTTON_GUIDE, pad_extra::kGuide},
        {SDL_GAMEPAD_BUTTON_MISC1, pad_extra::kMisc},
        {SDL_GAMEPAD_BUTTON_TOUCHPAD, pad_extra::kTouchpad},
    };
    std::uint8_t extra = 0;
    for (const auto& [button, bit] : kExtras) {
        if (SDL_GetGamepadButton(pad, button)) {
            extra = static_cast<std::uint8_t>(extra | bit);
        }
    }
    return {.connected = true,
            .buttons = buttons,
            .leftTrigger = trigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER),
            .rightTrigger = trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER),
            .leftX = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX),
            .leftY = upward(SDL_GAMEPAD_AXIS_LEFTY),
            .rightX = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTX),
            .rightY = upward(SDL_GAMEPAD_AXIS_RIGHTY),
            .extra = extra,
            .kind = kindOf(SDL_GetGamepadType(pad))};
}
#endif

void onMain(void (^block)())
{
    if ([NSThread isMainThread]) {
        block();
    } else {
        dispatch_sync(dispatch_get_main_queue(), block);
    }
}

}

struct InputCapture::Impl {
    InputHandler& handler;
    const Microseconds pollInterval;
    std::mutex mutex;
    std::condition_variable ready;
    CFRunLoopRef loop = nullptr;
    CFRunLoopTimerRef timer = nullptr;
    std::deque<std::function<void()>> tasks;
    std::string problem;
    bool stopping = false;
    std::thread thread;

    // The input thread's.
    std::optional<Microseconds> nextPoll;
    PadState lastPad;
#if defined(GS_HAVE_SDL3)
    bool padsStarted = false;
    SDL_Gamepad* pad = nullptr;
#endif

    // The main thread's: the event monitor and what it has seen.
    id monitor = nil;
    NSView* target = nil;
    std::bitset<128> held;
    double moveX = 0.0;
    double moveY = 0.0;
    double wheelY = 0.0;
    double wheelX = 0.0;

    Impl(InputHandler& given, Microseconds interval) : handler(given), pollInterval(interval) {}

    void fail(std::string what)
    {
        const std::scoped_lock lock(mutex);
        if (problem.empty()) {
            problem = std::move(what);
        }
    }

    // --- the input thread ---

    void arm()
    {
        std::optional<Microseconds> due = handler.nextWakeup();
        if (nextPoll && (!due || *nextPoll < *due)) {
            due = nextPoll;
        }
        if (!due) {
            CFRunLoopTimerSetNextFireDate(timer, CFAbsoluteTimeGetCurrent() + kNever);
            return;
        }
        const double seconds = std::chrono::duration<double>(*due - InputCapture::now()).count();
        CFRunLoopTimerSetNextFireDate(timer, CFAbsoluteTimeGetCurrent() + std::max(seconds, 0.0));
    }

    void onTimer()
    {
        const Microseconds now = InputCapture::now();
        if (nextPoll && now >= *nextPoll) {
            pollPads(now);
        }
        if (const auto due = handler.nextWakeup(); due && now >= *due) {
            handler.wake(now);
        }
        arm();
    }

    void runTasks()
    {
        std::deque<std::function<void()>> taken;
        {
            const std::scoped_lock lock(mutex);
            taken.swap(tasks);
        }
        for (std::function<void()>& task : taken) {
            task();
        }
        arm();
    }

    void startPads()
    {
#if defined(GS_HAVE_SDL3)
        SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
            fail(std::string("could not start gamepad support: ") + SDL_GetError());
            return;
        }
        SDL_SetJoystickEventsEnabled(false);
        SDL_SetGamepadEventsEnabled(false);
        padsStarted = true;
        nextPoll = InputCapture::now();
#endif
    }

    void stopPads()
    {
#if defined(GS_HAVE_SDL3)
        if (pad != nullptr) {
            SDL_CloseGamepad(pad);
            pad = nullptr;
        }
        if (padsStarted) {
            SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
            padsStarted = false;
        }
#endif
        nextPoll.reset();
    }

    // With no pad open, looks for one now and then (SDL notices arrivals as it updates); with one, reads it.
    void pollPads(Microseconds now)
    {
#if defined(GS_HAVE_SDL3)
        SDL_UpdateGamepads();
        if (pad != nullptr && !SDL_GamepadConnected(pad)) {
            SDL_CloseGamepad(pad);
            pad = nullptr;
            if (lastPad.connected) {
                lastPad = {};
                handler.gamepad(lastPad, now);
            }
        }
        if (pad == nullptr) {
            int count = 0;
            if (SDL_JoystickID* ids = SDL_GetGamepads(&count)) {
                for (int i = 0; i < count && pad == nullptr; ++i) {
                    pad = SDL_OpenGamepad(ids[i]);
                }
                SDL_free(ids);
            }
        }
        if (pad == nullptr) {
            nextPoll = now + kProbeInterval;
            return;
        }
        const PadState current = padStateOf(pad);
        if (current != lastPad) {
            lastPad = current;
            handler.gamepad(current, now);
        }
        nextPoll = now + pollInterval;
#else
        static_cast<void>(now);
        nextPoll.reset();
#endif
    }

    void rumble(std::uint16_t low, std::uint16_t high, std::uint32_t durationMs)
    {
#if defined(GS_HAVE_SDL3)
        if (pad != nullptr) {
            SDL_RumbleGamepad(pad, low, high, durationMs);
        }
#else
        static_cast<void>(low), static_cast<void>(high), static_cast<void>(durationMs);
#endif
    }

    void rumbleTriggers(std::uint16_t left, std::uint16_t right, std::uint32_t durationMs)
    {
#if defined(GS_HAVE_SDL3)
        if (pad != nullptr && SDL_GetBooleanProperty(SDL_GetGamepadProperties(pad),
                                                     SDL_PROP_GAMEPAD_CAP_TRIGGER_RUMBLE_BOOLEAN, false)) {
            SDL_RumbleGamepadTriggers(pad, left, right, durationMs);
        }
#else
        static_cast<void>(left), static_cast<void>(right), static_cast<void>(durationMs);
#endif
    }

    void run()
    {
        setCurrentThreadName("gs-input");
        setCurrentThreadRole(ThreadRole::Input);
        CFRunLoopTimerContext context{0, this, nullptr, nullptr, nullptr};
        timer = CFRunLoopTimerCreate(
            nullptr, CFAbsoluteTimeGetCurrent() + kNever, kNever, 0, 0,
            [](CFRunLoopTimerRef, void* info) { static_cast<Impl*>(info)->onTimer(); }, &context);
        CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
        {
            const std::scoped_lock lock(mutex);
            loop = CFRunLoopGetCurrent();
            CFRetain(loop);
        }
        ready.notify_all();
        startPads();
        runTasks();
        while (true) {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, false);
            const std::scoped_lock lock(mutex);
            if (stopping) {
                break;
            }
        }
        stopPads();
        CFRunLoopTimerInvalidate(timer);
        CFRelease(timer);
    }

    void post(std::function<void()> task)
    {
        CFRunLoopRef into = nullptr;
        {
            const std::scoped_lock lock(mutex);
            tasks.push_back(std::move(task));
            into = loop;
        }
        if (into != nullptr) {
            CFRunLoopPerformBlock(into, kCFRunLoopCommonModes, ^{
                runTasks();
            });
            CFRunLoopWakeUp(into);
        }
    }

    // --- the main thread ---

    void sendKey(std::uint16_t key, bool released, Microseconds now)
    {
        const std::optional<std::uint16_t> scanCode = mac::scanCodeOfKey(key);
        if (!scanCode) {
            return;
        }
        if (key < held.size()) {
            held[key] = !released;
        }
        const RawKey raw{.makeCode = static_cast<std::uint16_t>(*scanCode & 0xFF),
                         .e0 = (*scanCode & mac::kExtended) != 0,
                         .released = released};
        post([this, raw, now] { handler.key(raw, now); });
    }

    void sendMouse(const RawMouse& report, Microseconds now)
    {
        post([this, report, now] { handler.mouse(report, now); });
    }

    // Whole steps out of an accumulated amount, the rest kept for the next event.
    static int takeSteps(double& amount, double perStep)
    {
        const double steps = std::trunc(amount / perStep);
        amount -= steps * perStep;
        return static_cast<int>(std::clamp(steps, -static_cast<double>(kMostNotches), static_cast<double>(kMostNotches)));
    }

    void wheel(NSEvent* event, Microseconds now)
    {
        if (event.momentumPhase != NSEventPhaseNone) {
            return;
        }
        // Away from the user (up, or right for the horizontal wheel) is positive on Windows, whichever way the
        // user scrolls content ("natural" scrolling inverts the deltas).
        const double sign = event.isDirectionInvertedFromDevice ? -1.0 : 1.0;
        const bool precise = event.hasPreciseScrollingDeltas;
        const double perStep = precise ? kPointsPerStep : 1.0;
        wheelY += sign * (precise ? event.scrollingDeltaY : event.deltaY);
        wheelX -= sign * (precise ? event.scrollingDeltaX : event.deltaX);
        if (const int steps = takeSteps(wheelY, perStep); steps != 0) {
            sendMouse({.buttonFlags = kWheel, .buttonData = static_cast<std::int16_t>(steps * kWheelStep)}, now);
        }
        if (const int steps = takeSteps(wheelX, perStep); steps != 0) {
            sendMouse({.buttonFlags = kHorizontalWheel, .buttonData = static_cast<std::int16_t>(steps * kWheelStep)},
                      now);
        }
    }

    void move(NSEvent* event, Microseconds now)
    {
        moveX += event.deltaX;
        moveY += event.deltaY;
        const double dx = std::trunc(moveX);
        const double dy = std::trunc(moveY);
        moveX -= dx;
        moveY -= dy;
        if (dx != 0.0 || dy != 0.0) {
            sendMouse({.dx = static_cast<std::int32_t>(dx), .dy = static_cast<std::int32_t>(dy)}, now);
        }
    }

    static std::uint16_t buttonFlag(NSInteger button, bool down)
    {
        switch (button) {
        case 0: return down ? kLeftDown : kLeftUp;
        case 1: return down ? kRightDown : kRightUp;
        case 2: return down ? kMiddleDown : kMiddleUp;
        case 3: return down ? kX1Down : kX1Up;
        case 4: return down ? kX2Down : kX2Up;
        default: return 0;
        }
    }

    void seen(NSEvent* event)
    {
        if (target == nil || ![NSApp isActive] || ![[target window] isKeyWindow]) {
            return;
        }
        const Microseconds now = InputCapture::now();
        switch (event.type) {
        case NSEventTypeKeyDown:
            if (!event.isARepeat) {
                sendKey(event.keyCode, false, now);
            }
            break;
        case NSEventTypeKeyUp:
            sendKey(event.keyCode, true, now);
            break;
        case NSEventTypeFlagsChanged:
            if (event.keyCode == 0x39) { // caps lock: one event per press, whichever way it turns
                sendKey(event.keyCode, false, now);
                sendKey(event.keyCode, true, now);
            } else if (const std::optional<bool> down = modifierDown(event.keyCode, event.modifierFlags)) {
                sendKey(event.keyCode, !*down, now);
            }
            break;
        case NSEventTypeLeftMouseDown:
        case NSEventTypeRightMouseDown:
        case NSEventTypeOtherMouseDown:
            if (const std::uint16_t flag = buttonFlag(event.buttonNumber, true)) {
                sendMouse({.buttonFlags = flag}, now);
            }
            break;
        case NSEventTypeLeftMouseUp:
        case NSEventTypeRightMouseUp:
        case NSEventTypeOtherMouseUp:
            if (const std::uint16_t flag = buttonFlag(event.buttonNumber, false)) {
                sendMouse({.buttonFlags = flag}, now);
            }
            break;
        case NSEventTypeMouseMoved:
        case NSEventTypeLeftMouseDragged:
        case NSEventTypeRightMouseDragged:
        case NSEventTypeOtherMouseDragged:
            move(event, now);
            break;
        case NSEventTypeScrollWheel:
            wheel(event, now);
            break;
        default:
            break;
        }
    }

    // Keys still down when the capture stops are let go, so none stays pressed on the phone.
    void releaseHeld()
    {
        const Microseconds now = InputCapture::now();
        for (std::uint16_t key = 0; key < held.size(); ++key) {
            if (held[key]) {
                sendKey(key, true, now);
            }
        }
        held.reset();
        moveX = moveY = wheelX = wheelY = 0.0;
    }

    void watch(std::uintptr_t window)
    {
        NSView* view = (__bridge NSView*)reinterpret_cast<void*>(window);
        if (view == target) {
            return;
        }
        if (target != nil) {
            releaseHeld();
        }
        target = view;
        if (target != nil && monitor == nil) {
            constexpr NSEventMask kMask =
                NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskFlagsChanged | NSEventMaskLeftMouseDown |
                NSEventMaskLeftMouseUp | NSEventMaskRightMouseDown | NSEventMaskRightMouseUp |
                NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp | NSEventMaskMouseMoved |
                NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged |
                NSEventMaskScrollWheel;
            Impl* self = this;
            monitor = [NSEvent addLocalMonitorForEventsMatchingMask:kMask
                                                            handler:^NSEvent*(NSEvent* event) {
                                                                self->seen(event);
                                                                return event;
                                                            }];
        } else if (target == nil && monitor != nil) {
            [NSEvent removeMonitor:monitor];
            monitor = nil;
        }
    }
};

InputCapture::InputCapture(InputHandler& handler, Microseconds pollInterval)
    : m_impl(std::make_unique<Impl>(handler, pollInterval))
{
    Impl* impl = m_impl.get();
    impl->thread = std::thread([impl] { impl->run(); });
    std::unique_lock lock(impl->mutex);
    impl->ready.wait(lock, [impl] { return impl->loop != nullptr; });
}

InputCapture::~InputCapture()
{
    Impl* impl = m_impl.get();
    onMain(^{
        impl->target = nil;
        if (impl->monitor != nil) {
            [NSEvent removeMonitor:impl->monitor];
            impl->monitor = nil;
        }
    });
    CFRunLoopRef loop = nullptr;
    {
        const std::scoped_lock lock(impl->mutex);
        impl->stopping = true;
        loop = impl->loop;
    }
    CFRunLoopStop(loop);
    CFRunLoopWakeUp(loop);
    impl->thread.join();
    CFRelease(loop);
}

bool InputCapture::started() const noexcept
{
    return true;
}

std::string InputCapture::problem() const
{
    const std::scoped_lock lock(m_impl->mutex);
    return m_impl->problem;
}

Microseconds InputCapture::now() noexcept
{
    return SteadyClock{}.now();
}

void InputCapture::post(std::function<void()> task)
{
    m_impl->post(std::move(task));
}

void InputCapture::setCaptureTarget(std::uintptr_t window)
{
    Impl* impl = m_impl.get();
    if ([NSThread isMainThread]) {
        impl->watch(window);
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            impl->watch(window);
        });
    }
}

void InputCapture::rumble(std::uint16_t low, std::uint16_t high, std::uint32_t durationMs)
{
    post([impl = m_impl.get(), low, high, durationMs] { impl->rumble(low, high, durationMs); });
}

void InputCapture::rumbleTriggers(std::uint16_t left, std::uint16_t right, std::uint32_t durationMs)
{
    post([impl = m_impl.get(), left, right, durationMs] { impl->rumbleTriggers(left, right, durationMs); });
}

}
