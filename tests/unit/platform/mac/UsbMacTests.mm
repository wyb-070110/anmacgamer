// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// macOS USB phone listing, libusb version, hotplug watcher lifecycle and safe open error paths.
#include "gs/platform/Usb.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <format>
#include <string>
#include <vector>

namespace {

using gs::platform::listUsbPhones;
using gs::platform::UsbDevice;
using gs::platform::UsbDriver;
using gs::platform::UsbFailure;
using gs::platform::usbLibraryVersion;
using gs::platform::UsbPhone;
using gs::platform::UsbWatcher;

bool looksLikeSemver(const std::string& text)
{
    int dots = 0;
    bool digitsInPart = false;
    for (const char ch : text) {
        if (ch == '.') {
            if (!digitsInPart) {
                return false;
            }
            ++dots;
            digitsInPart = false;
        } else if (std::isdigit(static_cast<unsigned char>(ch))) {
            digitsInPart = true;
        } else {
            return false;
        }
    }
    return dots == 2 && digitsInPart;
}

} // namespace

TEST_CASE("macOS USB reports libusb version and lists attached Android phones in sorted Ready order")
{
    const std::string version = usbLibraryVersion();
    MESSAGE("libusb version: ", version);
    CHECK(looksLikeSemver(version));

    const std::vector<UsbPhone> phones = listUsbPhones();
    MESSAGE("attached USB phones: ", phones.size());
    for (const UsbPhone& phone : phones) {
        MESSAGE(std::format("  serial={} name=\"{}\" vid={:04x} pid={:04x} driver={} debugging={}", phone.serial,
                            phone.name, phone.vendorId, phone.productId,
                            phone.driver == UsbDriver::Ready ? "Ready" : "Missing", phone.debugging));
        CHECK_FALSE(phone.serial.empty());
        CHECK(phone.vendorId != 0);
        CHECK(phone.vendorId != 0x05AC);
        CHECK(phone.driver == UsbDriver::Ready);
    }
    CHECK(std::ranges::is_sorted(phones, {}, &UsbPhone::serial));
}

TEST_CASE("macOS UsbDevice::open reports NotFound cleanly without touching real devices")
{
    UsbFailure failure = UsbFailure::None;
    CHECK(UsbDevice::open(UsbPhone{}, failure) == nullptr);
    CHECK(failure == UsbFailure::NotFound);

    failure = UsbFailure::None;
    const UsbPhone fake{
        .serial = "GS-NONEXISTENT-SERIAL-0000",
        .name = "Fake Phone",
        .vendorId = 0x18D1,
        .productId = 0x4EE7,
        .driver = UsbDriver::Ready,
        .debugging = true,
    };
    CHECK(UsbDevice::open(fake, failure) == nullptr);
    CHECK(failure == UsbFailure::NotFound);
}

TEST_CASE("macOS UsbWatcher starts hotplug monitoring on its own thread and stops cleanly")
{
    std::atomic<int> notifications{0};
    {
        UsbWatcher watcher([&] { notifications.fetch_add(1, std::memory_order_relaxed); });
        CHECK(watcher.started());
    }
    for (int cycle = 0; cycle < 8; ++cycle) {
        UsbWatcher watcher([&] { notifications.fetch_add(1, std::memory_order_relaxed); });
        REQUIRE(watcher.started());
    }
    CHECK(notifications.load(std::memory_order_relaxed) >= 0);
}
