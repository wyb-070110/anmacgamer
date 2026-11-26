// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The per-user key behind protectForUser: created on first use, then read back.
#pragma once

#include "gs/platform/Crypto.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>

namespace gs::platform {

using UserKey = std::array<std::uint8_t, kAesGcmKeySize>;

namespace detail {

[[nodiscard]] std::optional<UserKey> userKey() noexcept;

// The key kept in a file of the user's own (mode 0600, in a 0700 folder), created there on first use.
[[nodiscard]] std::optional<UserKey> userKeyInFile(const std::filesystem::path& path) noexcept;

}

}
