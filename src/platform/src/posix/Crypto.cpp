// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// POSIX cryptography on OpenSSL libcrypto (Homebrew openssl@3 on macOS). protectForUser lives with the
// keychain code of each system.
#include "gs/platform/Crypto.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/params.h>
#include <openssl/rand.h>

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <memory>

namespace gs::platform {

namespace {

void require(bool ok) noexcept
{
    if (!ok) {
        std::abort();
    }
}

int length(std::span<const std::uint8_t> bytes) noexcept
{
    require(bytes.size() <= static_cast<std::size_t>(INT_MAX));
    return static_cast<int>(bytes.size());
}

struct CipherFree {
    void operator()(EVP_CIPHER_CTX* context) const noexcept { EVP_CIPHER_CTX_free(context); }
};
using Cipher = std::unique_ptr<EVP_CIPHER_CTX, CipherFree>;

struct MacFree {
    void operator()(EVP_MAC* mac) const noexcept { EVP_MAC_free(mac); }
};
struct MacContextFree {
    void operator()(EVP_MAC_CTX* context) const noexcept { EVP_MAC_CTX_free(context); }
};

Cipher gcm(std::span<const std::uint8_t, kAesGcmKeySize> key, std::span<const std::uint8_t, kAesGcmNonceSize> nonce,
           bool encrypt) noexcept
{
    Cipher context(EVP_CIPHER_CTX_new());
    require(context != nullptr);
    const auto init = encrypt ? EVP_EncryptInit_ex : EVP_DecryptInit_ex;
    require(init(context.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1);
    require(EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) == 1);
    require(init(context.get(), nullptr, nullptr, key.data(), nonce.data()) == 1);
    return context;
}

}

Sha256Digest sha256(std::span<const std::uint8_t> data)
{
    Sha256Digest digest{};
    unsigned int size = 0;
    require(EVP_Digest(data.data(), data.size(), digest.data(), &size, EVP_sha256(), nullptr) == 1);
    require(size == digest.size());
    return digest;
}

Sha256Digest hmacSha256(std::span<const std::uint8_t> key, std::initializer_list<std::span<const std::uint8_t>> parts)
{
    static const std::unique_ptr<EVP_MAC, MacFree> mac(EVP_MAC_fetch(nullptr, "HMAC", nullptr));
    require(mac != nullptr);
    const std::unique_ptr<EVP_MAC_CTX, MacContextFree> context(EVP_MAC_CTX_new(mac.get()));
    require(context != nullptr);
    char digestName[] = "SHA256";
    const OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string("digest", digestName, 0),
        OSSL_PARAM_construct_end(),
    };
    static constexpr std::uint8_t kEmptyKey = 0;
    require(EVP_MAC_init(context.get(), key.empty() ? &kEmptyKey : key.data(), key.size(), params) == 1);
    for (const std::span<const std::uint8_t> part : parts) {
        if (!part.empty()) {
            require(EVP_MAC_update(context.get(), part.data(), part.size()) == 1);
        }
    }
    Sha256Digest digest{};
    std::size_t size = 0;
    require(EVP_MAC_final(context.get(), digest.data(), &size, digest.size()) == 1);
    require(size == digest.size());
    return digest;
}

void randomBytes(std::span<std::uint8_t> out)
{
    if (!out.empty()) {
        require(RAND_bytes(out.data(), length(out)) == 1);
    }
}

std::string toHex(std::span<const std::uint8_t> bytes)
{
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string text;
    text.reserve(bytes.size() * 2);
    for (const std::uint8_t byte : bytes) {
        text.push_back(kDigits[byte >> 4U]);
        text.push_back(kDigits[byte & 0x0FU]);
    }
    return text;
}

std::vector<std::uint8_t> aesGcmSeal(std::span<const std::uint8_t, kAesGcmKeySize> key,
                                     std::span<const std::uint8_t, kAesGcmNonceSize> nonce,
                                     std::span<const std::uint8_t> plaintext, std::span<const std::uint8_t> aad)
{
    const Cipher context = gcm(key, nonce, true);
    int written = 0;
    if (!aad.empty()) {
        require(EVP_EncryptUpdate(context.get(), nullptr, &written, aad.data(), length(aad)) == 1);
    }
    std::vector<std::uint8_t> sealed(plaintext.size() + kAesGcmTagSize);
    int total = 0;
    if (!plaintext.empty()) {
        require(EVP_EncryptUpdate(context.get(), sealed.data(), &written, plaintext.data(), length(plaintext)) == 1);
        total = written;
    }
    require(EVP_EncryptFinal_ex(context.get(), sealed.data() + total, &written) == 1);
    total += written;
    require(static_cast<std::size_t>(total) == plaintext.size());
    require(EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_GET_TAG, static_cast<int>(kAesGcmTagSize),
                                sealed.data() + total) == 1);
    return sealed;
}

std::optional<std::vector<std::uint8_t>> aesGcmOpen(std::span<const std::uint8_t, kAesGcmKeySize> key,
                                                    std::span<const std::uint8_t, kAesGcmNonceSize> nonce,
                                                    std::span<const std::uint8_t> sealed,
                                                    std::span<const std::uint8_t> aad)
{
    if (sealed.size() < kAesGcmTagSize) {
        return std::nullopt;
    }
    const std::span<const std::uint8_t> ciphertext = sealed.first(sealed.size() - kAesGcmTagSize);
    std::array<std::uint8_t, kAesGcmTagSize> tag{};
    std::ranges::copy(sealed.last(kAesGcmTagSize), tag.begin());
    const Cipher context = gcm(key, nonce, false);
    int written = 0;
    if (!aad.empty()) {
        require(EVP_DecryptUpdate(context.get(), nullptr, &written, aad.data(), length(aad)) == 1);
    }
    std::vector<std::uint8_t> plaintext(ciphertext.size());
    int total = 0;
    if (!ciphertext.empty()) {
        require(EVP_DecryptUpdate(context.get(), plaintext.data(), &written, ciphertext.data(), length(ciphertext)) ==
                1);
        total = written;
    }
    require(EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), tag.data()) == 1);
    if (EVP_DecryptFinal_ex(context.get(), plaintext.data() + total, &written) != 1) {
        OPENSSL_cleanse(plaintext.data(), plaintext.size());
        return std::nullopt;
    }
    return plaintext;
}

}
