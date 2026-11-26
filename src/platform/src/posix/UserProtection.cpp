// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Data protected for the current user off Windows: AES-256-GCM under a per-user key, the entropy as
// associated data. The key lives in the login keychain on macOS and in a file only the user can read
// elsewhere (Linux serves as a test bed for the port).
#include "UserKey.h"

#include "gs/platform/Crypto.h"

#include <algorithm>
#include <array>

namespace gs::platform {

namespace {

constexpr std::array<std::uint8_t, 4> kMagic{'G', 'S', 'P', '1'};

}

std::optional<std::vector<std::uint8_t>> protectForUser(std::span<const std::uint8_t> secret,
                                                        std::span<const std::uint8_t> entropy)
{
    const std::optional<UserKey> key = detail::userKey();
    if (!key) {
        return std::nullopt;
    }
    std::array<std::uint8_t, kAesGcmNonceSize> nonce{};
    randomBytes(nonce);
    const std::vector<std::uint8_t> sealed = aesGcmSeal(*key, nonce, secret, entropy);
    std::vector<std::uint8_t> blob;
    blob.reserve(kMagic.size() + nonce.size() + sealed.size());
    blob.insert(blob.end(), kMagic.begin(), kMagic.end());
    blob.insert(blob.end(), nonce.begin(), nonce.end());
    blob.insert(blob.end(), sealed.begin(), sealed.end());
    return blob;
}

std::optional<std::vector<std::uint8_t>> unprotectForUser(std::span<const std::uint8_t> blob,
                                                          std::span<const std::uint8_t> entropy)
{
    if (blob.size() < kMagic.size() + kAesGcmNonceSize + kAesGcmTagSize ||
        !std::ranges::equal(blob.first(kMagic.size()), kMagic)) {
        return std::nullopt;
    }
    const std::optional<UserKey> key = detail::userKey();
    if (!key) {
        return std::nullopt;
    }
    const auto nonce = blob.subspan(kMagic.size()).first<kAesGcmNonceSize>();
    return aesGcmOpen(*key, nonce, blob.subspan(kMagic.size() + kAesGcmNonceSize), entropy);
}

}
