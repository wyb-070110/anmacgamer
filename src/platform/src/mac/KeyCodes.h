// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
#pragma once

// macOS virtual key codes (kVK_*, as in Carbon's Events.h) and the PC set-1 scan codes the program names keys by
// everywhere (profiles, key maps): a scan code of an extended key is 0xE000 | its make code. Keys are matched by
// position, so a profile made on Windows presses the same keys on a Mac.

#include <array>
#include <cstdint>
#include <optional>

namespace gs::platform::mac {

inline constexpr std::uint16_t kExtended = 0xE000;

// Index: macOS virtual key code (0-127). 0: no PC key there (the Fn key, unused codes).
inline constexpr std::array<std::uint16_t, 128> kScanCodeOfKey = [] {
    std::array<std::uint16_t, 128> table{};
    const auto set = [&table](std::uint8_t key, std::uint16_t scanCode) { table[key] = scanCode; };
    set(0x00, 0x1E); // A
    set(0x01, 0x1F); // S
    set(0x02, 0x20); // D
    set(0x03, 0x21); // F
    set(0x04, 0x23); // H
    set(0x05, 0x22); // G
    set(0x06, 0x2C); // Z
    set(0x07, 0x2D); // X
    set(0x08, 0x2E); // C
    set(0x09, 0x2F); // V
    set(0x0A, 0x56); // ISO section, the key left of Z on ISO keyboards
    set(0x0B, 0x30); // B
    set(0x0C, 0x10); // Q
    set(0x0D, 0x11); // W
    set(0x0E, 0x12); // E
    set(0x0F, 0x13); // R
    set(0x10, 0x15); // Y
    set(0x11, 0x14); // T
    set(0x12, 0x02); // 1
    set(0x13, 0x03); // 2
    set(0x14, 0x04); // 3
    set(0x15, 0x05); // 4
    set(0x16, 0x07); // 6
    set(0x17, 0x06); // 5
    set(0x18, 0x0D); // =
    set(0x19, 0x0A); // 9
    set(0x1A, 0x08); // 7
    set(0x1B, 0x0C); // -
    set(0x1C, 0x09); // 8
    set(0x1D, 0x0B); // 0
    set(0x1E, 0x1B); // ]
    set(0x1F, 0x18); // O
    set(0x20, 0x16); // U
    set(0x21, 0x1A); // [
    set(0x22, 0x17); // I
    set(0x23, 0x19); // P
    set(0x24, 0x1C); // Return
    set(0x25, 0x26); // L
    set(0x26, 0x24); // J
    set(0x27, 0x28); // '
    set(0x28, 0x25); // K
    set(0x29, 0x27); // ;
    set(0x2A, 0x2B); // backslash
    set(0x2B, 0x33); // ,
    set(0x2C, 0x35); // /
    set(0x2D, 0x31); // N
    set(0x2E, 0x32); // M
    set(0x2F, 0x34); // .
    set(0x30, 0x0F); // Tab
    set(0x31, 0x39); // Space
    set(0x32, 0x29); // `
    set(0x33, 0x0E); // Delete (backspace)
    set(0x35, 0x01); // Escape
    set(0x36, kExtended | 0x5C); // right Command: right Windows key
    set(0x37, kExtended | 0x5B); // Command: left Windows key
    set(0x38, 0x2A); // Shift
    set(0x39, 0x3A); // Caps Lock
    set(0x3A, 0x38); // Option: Alt
    set(0x3B, 0x1D); // Control
    set(0x3C, 0x36); // right Shift
    set(0x3D, kExtended | 0x38); // right Option: right Alt
    set(0x3E, kExtended | 0x1D); // right Control
    set(0x40, 0x68); // F17
    set(0x41, 0x53); // keypad .
    set(0x43, 0x37); // keypad *
    set(0x45, 0x4E); // keypad +
    set(0x47, 0x45); // keypad Clear: Num Lock
    set(0x48, kExtended | 0x30); // volume up
    set(0x49, kExtended | 0x2E); // volume down
    set(0x4A, kExtended | 0x20); // mute
    set(0x4B, kExtended | 0x35); // keypad /
    set(0x4C, kExtended | 0x1C); // keypad Enter
    set(0x4E, 0x4A); // keypad -
    set(0x4F, 0x69); // F18
    set(0x50, 0x6A); // F19
    set(0x51, 0x59); // keypad =
    set(0x52, 0x52); // keypad 0
    set(0x53, 0x4F); // keypad 1
    set(0x54, 0x50); // keypad 2
    set(0x55, 0x51); // keypad 3
    set(0x56, 0x4B); // keypad 4
    set(0x57, 0x4C); // keypad 5
    set(0x58, 0x4D); // keypad 6
    set(0x59, 0x47); // keypad 7
    set(0x5A, 0x6B); // F20
    set(0x5B, 0x48); // keypad 8
    set(0x5C, 0x49); // keypad 9
    set(0x5D, 0x7D); // JIS yen
    set(0x5E, 0x73); // JIS underscore
    set(0x5F, 0x7E); // JIS keypad comma
    set(0x60, 0x3F); // F5
    set(0x61, 0x40); // F6
    set(0x62, 0x41); // F7
    set(0x63, 0x3D); // F3
    set(0x64, 0x42); // F8
    set(0x65, 0x43); // F9
    set(0x66, 0x7B); // JIS Eisu: Muhenkan
    set(0x67, 0x57); // F11
    set(0x68, 0x70); // JIS Kana: Katakana/Hiragana
    set(0x69, 0x64); // F13
    set(0x6A, 0x67); // F16
    set(0x6B, 0x65); // F14
    set(0x6D, 0x44); // F10
    set(0x6E, kExtended | 0x5D); // context menu
    set(0x6F, 0x58); // F12
    set(0x71, 0x66); // F15
    set(0x72, kExtended | 0x52); // Help: Insert
    set(0x73, kExtended | 0x47); // Home
    set(0x74, kExtended | 0x49); // Page Up
    set(0x75, kExtended | 0x53); // forward delete: Delete
    set(0x76, 0x3E); // F4
    set(0x77, kExtended | 0x4F); // End
    set(0x78, 0x3C); // F2
    set(0x79, kExtended | 0x51); // Page Down
    set(0x7A, 0x3B); // F1
    set(0x7B, kExtended | 0x4B); // left arrow
    set(0x7C, kExtended | 0x4D); // right arrow
    set(0x7D, kExtended | 0x50); // down arrow
    set(0x7E, kExtended | 0x48); // up arrow
    return table;
}();

[[nodiscard]] inline std::optional<std::uint16_t> scanCodeOfKey(std::uint16_t key) noexcept
{
    if (key >= kScanCodeOfKey.size() || kScanCodeOfKey[key] == 0) {
        return std::nullopt;
    }
    return kScanCodeOfKey[key];
}

[[nodiscard]] inline std::optional<std::uint16_t> keyOfScanCode(std::uint16_t scanCode) noexcept
{
    for (std::uint16_t key = 0; key < kScanCodeOfKey.size(); ++key) {
        if (kScanCodeOfKey[key] == scanCode) {
            return key;
        }
    }
    return std::nullopt;
}

}
