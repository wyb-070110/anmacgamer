// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The per-user key as a generic password in the user's login keychain, for the app in its bundle.
//
// A program outside an app bundle (the test programs, development tools) keeps its own key in a file
// instead. The login keychain lets only the program that created an item read it without asking, and it
// tells programs apart by their code: a rebuilt or differently built test program would make macOS show
// an access dialog, and the test would wait for an answer that never comes. The data protection keychain
// has no such dialog but needs a signed app with entitlements (errSecMissingEntitlement otherwise).
#include "UserKey.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <mach-o/dyld.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <mutex>
#include <string>
#include <vector>

namespace gs::platform::detail {

namespace {

constexpr char kService[] = "GameScrcpy";
constexpr char kAccount[] = "user-protection-key";

class Ref
{
public:
    explicit Ref(CFTypeRef ref = nullptr) noexcept : m_ref(ref) {}
    ~Ref()
    {
        if (m_ref != nullptr) {
            CFRelease(m_ref);
        }
    }
    Ref(const Ref&) = delete;
    Ref& operator=(const Ref&) = delete;

    [[nodiscard]] CFTypeRef get() const noexcept { return m_ref; }
    CFTypeRef* out() noexcept { return &m_ref; }

private:
    CFTypeRef m_ref;
};

Ref string(const char* text) noexcept
{
    return Ref(CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8));
}

Ref query(const Ref& service, const Ref& account) noexcept
{
    CFMutableDictionaryRef dictionary =
        CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (dictionary != nullptr) {
        CFDictionarySetValue(dictionary, kSecClass, kSecClassGenericPassword);
        CFDictionarySetValue(dictionary, kSecAttrService, service.get());
        CFDictionarySetValue(dictionary, kSecAttrAccount, account.get());
    }
    return Ref(dictionary);
}

std::optional<UserKey> find(const Ref& service, const Ref& account) noexcept
{
    const Ref search = query(service, account);
    if (search.get() == nullptr) {
        return std::nullopt;
    }
    auto* dictionary = static_cast<CFMutableDictionaryRef>(const_cast<void*>(search.get()));
    CFDictionarySetValue(dictionary, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(dictionary, kSecMatchLimit, kSecMatchLimitOne);
    Ref data;
    if (SecItemCopyMatching(dictionary, data.out()) != errSecSuccess || data.get() == nullptr ||
        CFGetTypeID(data.get()) != CFDataGetTypeID()) {
        return std::nullopt;
    }
    const auto bytes = static_cast<CFDataRef>(data.get());
    UserKey key{};
    if (static_cast<std::size_t>(CFDataGetLength(bytes)) != key.size()) {
        return std::nullopt;
    }
    std::memcpy(key.data(), CFDataGetBytePtr(bytes), key.size());
    return key;
}

std::optional<UserKey> add(const Ref& service, const Ref& account) noexcept
{
    UserKey key{};
    randomBytes(key);
    const Ref item = query(service, account);
    const Ref data(CFDataCreate(kCFAllocatorDefault, key.data(), static_cast<CFIndex>(key.size())));
    if (item.get() == nullptr || data.get() == nullptr) {
        return std::nullopt;
    }
    auto* dictionary = static_cast<CFMutableDictionaryRef>(const_cast<void*>(item.get()));
    CFDictionarySetValue(dictionary, kSecValueData, data.get());
    CFDictionarySetValue(dictionary, kSecAttrAccessible, kSecAttrAccessibleWhenUnlockedThisDeviceOnly);
    const OSStatus status = SecItemAdd(dictionary, nullptr);
    if (status == errSecDuplicateItem) {
        return find(service, account);
    }
    return status == errSecSuccess ? std::optional<UserKey>(key) : std::nullopt;
}

bool inAppBundle() noexcept
{
    CFBundleRef bundle = CFBundleGetMainBundle();
    return bundle != nullptr && CFBundleGetIdentifier(bundle) != nullptr;
}

// ~/Library/Application Support/anmacgamer/Unbundled/<program>-<hash of its full path>.key, so a debug and
// a release build of the same program never share a key.
std::filesystem::path unbundledKeyFile()
{
    const char* home = std::getenv("HOME");
    if (home == nullptr || *home != '/') {
        return {};
    }
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> raw(size + 1, '\0');
    if (size == 0 || _NSGetExecutablePath(raw.data(), &size) != 0) {
        return {};
    }
    std::error_code error;
    std::filesystem::path program = std::filesystem::weakly_canonical(raw.data(), error);
    if (error) {
        program = raw.data();
    }
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : program.string()) {
        hash = (hash ^ static_cast<unsigned char>(c)) * 0x100000001b3ULL;
    }
    return std::filesystem::path(home) / "Library" / "Application Support" / "anmacgamer" / "Unbundled" /
           std::format("{}-{:016x}.key", program.filename().string(), hash);
}

}

std::optional<UserKey> userKey() noexcept
{
    if (!inAppBundle()) {
        try {
            return userKeyInFile(unbundledKeyFile());
        } catch (...) {
            return std::nullopt;
        }
    }
    // Read once per process: every read of a login keychain item may show macOS's access dialog (each time, when
    // the user picks "Allow" rather than "Always Allow"), and the key is asked for on every protect and unprotect.
    static std::mutex mutex;
    static std::optional<UserKey> known;
    const std::lock_guard lock(mutex);
    if (known) {
        return known;
    }
    const Ref service = string(kService);
    const Ref account = string(kAccount);
    if (service.get() == nullptr || account.get() == nullptr) {
        return std::nullopt;
    }
    known = find(service, account);
    if (!known) {
        known = add(service, account);
    }
    return known;
}

}
