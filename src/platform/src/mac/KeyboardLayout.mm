// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The character a key types with no modifier in the current keyboard layout, on macOS (UCKeyTranslate).
#include "gs/platform/KeyboardLayout.h"

#include "KeyCodes.h"

#include <Carbon/Carbon.h>

namespace gs::platform {

std::optional<char32_t> layoutCharacter(std::uint16_t scanCode) noexcept
{
    const std::optional<std::uint16_t> key = mac::keyOfScanCode(scanCode);
    if (!key) {
        return std::nullopt;
    }
    TISInputSourceRef source = TISCopyCurrentKeyboardLayoutInputSource();
    if (source == nullptr) {
        return std::nullopt;
    }
    const auto data = static_cast<CFDataRef>(TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData));
    std::optional<char32_t> typed;
    if (data != nullptr) {
        const auto* layout = reinterpret_cast<const UCKeyboardLayout*>(CFDataGetBytePtr(data));
        UInt32 deadKeys = 0;
        UniChar text[4] = {};
        UniCharCount length = 0;
        const OSStatus status = UCKeyTranslate(layout, *key, kUCKeyActionDisplay, 0, LMGetKbdType(),
                                               kUCKeyTranslateNoDeadKeysBit, &deadKeys, 4, &length, text);
        if (status == noErr && length >= 1) {
            char32_t c = text[0];
            if (length >= 2 && text[0] >= 0xD800 && text[0] < 0xDC00) {
                c = 0x10000 + ((static_cast<char32_t>(text[0]) - 0xD800) << 10) + (text[1] - 0xDC00);
            }
            if (c >= 0x21 && c != 0x7F) {
                typed = c;
            }
        }
    }
    CFRelease(source);
    return typed;
}

}
