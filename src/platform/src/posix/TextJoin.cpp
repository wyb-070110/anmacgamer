// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Word spacing for read text, shared by every reader off Windows.
#include "gs/platform/TextReader.h"

namespace gs::platform {

bool joinsWithSpace(char32_t before, char32_t after) noexcept
{
    const auto wide = [](char32_t point) {
        return (point >= 0x2E80 && point <= 0x9FFF) || (point >= 0xF900 && point <= 0xFAFF)
               || (point >= 0xFE30 && point <= 0xFE4F) || (point >= 0xFF00 && point <= 0xFFEF)
               || (point >= 0x20000 && point <= 0x3FFFF);
    };
    return before != 0 && after != 0 && !wide(before) && !wide(after);
}

}
