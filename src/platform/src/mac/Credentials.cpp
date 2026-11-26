// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Saved secrets on macOS: generic passwords in the user's login keychain, service "GameScrcpy", one account per
// target. Errors are OSStatus codes in a category of their own; a missing item is errSecItemNotFound.
#include "gs/platform/Credentials.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include <string>

namespace gs::platform {

namespace {

class KeychainCategory final : public std::error_category
{
public:
    [[nodiscard]] const char* name() const noexcept override { return "keychain"; }
    [[nodiscard]] std::string message(int code) const override
    {
        std::string text = "keychain error " + std::to_string(code);
        if (const CFStringRef described = SecCopyErrorMessageString(static_cast<OSStatus>(code), nullptr)) {
            char buffer[256] = {};
            if (CFStringGetCString(described, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
                text = buffer;
            }
            CFRelease(described);
        }
        return text;
    }
};

const std::error_category& keychain() noexcept
{
    static const KeychainCategory category;
    return category;
}

std::error_code errorOf(OSStatus status) noexcept
{
    return status == errSecSuccess ? std::error_code() : std::error_code(static_cast<int>(status), keychain());
}

CFStringRef text(std::string_view value) noexcept
{
    return CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(value.data()),
                                   static_cast<CFIndex>(value.size()), kCFStringEncodingUTF8, false);
}

CFMutableDictionaryRef query(std::string_view target) noexcept
{
    CFMutableDictionaryRef made =
        CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    const CFStringRef account = text(target);
    if (made == nullptr || account == nullptr) {
        if (made != nullptr) {
            CFRelease(made);
        }
        if (account != nullptr) {
            CFRelease(account);
        }
        return nullptr;
    }
    CFDictionarySetValue(made, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(made, kSecAttrService, CFSTR("GameScrcpy"));
    CFDictionarySetValue(made, kSecAttrAccount, account);
    CFRelease(account);
    return made;
}

}

std::error_code writeCredential(std::string_view target, std::string_view secret) noexcept
{
    if (secret.size() > kMostSecretBytes) {
        return std::make_error_code(std::errc::value_too_large);
    }
    CFMutableDictionaryRef found = query(target);
    const CFDataRef data =
        CFDataCreate(nullptr, reinterpret_cast<const UInt8*>(secret.data()), static_cast<CFIndex>(secret.size()));
    if (found == nullptr || data == nullptr) {
        if (found != nullptr) {
            CFRelease(found);
        }
        if (data != nullptr) {
            CFRelease(data);
        }
        return std::make_error_code(std::errc::not_enough_memory);
    }
    const void* keys[] = {kSecValueData};
    const void* values[] = {data};
    const CFDictionaryRef change =
        CFDictionaryCreate(nullptr, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    OSStatus status = SecItemUpdate(found, change);
    if (status == errSecItemNotFound) {
        CFDictionarySetValue(found, kSecValueData, data);
        status = SecItemAdd(found, nullptr);
    }
    CFRelease(change);
    CFRelease(data);
    CFRelease(found);
    return errorOf(status);
}

Expected<std::string, std::error_code> readCredential(std::string_view target) noexcept
{
    CFMutableDictionaryRef found = query(target);
    if (found == nullptr) {
        return Unexpected<std::error_code>(std::make_error_code(std::errc::not_enough_memory));
    }
    CFDictionarySetValue(found, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(found, kSecMatchLimit, kSecMatchLimitOne);
    CFTypeRef result = nullptr;
    const OSStatus status = SecItemCopyMatching(found, &result);
    CFRelease(found);
    if (status != errSecSuccess) {
        return Unexpected<std::error_code>(errorOf(status));
    }
    std::string secret;
    if (result != nullptr && CFGetTypeID(result) == CFDataGetTypeID()) {
        const auto data = static_cast<CFDataRef>(result);
        secret.assign(reinterpret_cast<const char*>(CFDataGetBytePtr(data)), static_cast<std::size_t>(CFDataGetLength(data)));
    }
    if (result != nullptr) {
        CFRelease(result);
    }
    return secret;
}

std::error_code deleteCredential(std::string_view target) noexcept
{
    CFMutableDictionaryRef found = query(target);
    if (found == nullptr) {
        return std::make_error_code(std::errc::not_enough_memory);
    }
    const OSStatus status = SecItemDelete(found);
    CFRelease(found);
    return status == errSecItemNotFound ? std::error_code() : errorOf(status);
}

bool credentialMissing(const std::error_code& error) noexcept
{
    return error.category() == keychain() && error.value() == errSecItemNotFound;
}

}
