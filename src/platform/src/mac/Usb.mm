// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Android phones over USB on macOS: enumeration, hotplug watching and AOA control transfers via libusb.
#include "gs/platform/Usb.h"

#include "gs/platform/Thread.h"

#ifdef GS_HAVE_LIBUSB

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>

#include <libusb.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <format>
#include <string_view>
#include <thread>
#include <utility>

namespace gs::platform {

namespace {

constexpr unsigned kTransferTimeoutMs = 1000;
constexpr std::size_t kDefaultPacketSize = 64;
constexpr std::uint8_t kVendorToDevice = 0x40;
constexpr std::uint8_t kVendorFromDevice = 0xC0;
constexpr std::uint16_t kAppleVendor = 0x05AC;
constexpr std::uint16_t kGoogleVendor = 0x18D1;
constexpr std::uint16_t kFirstAccessory = 0x2D00;
constexpr std::uint16_t kLastAccessory = 0x2D05;

constexpr std::array<std::uint16_t, 30> kAndroidVendors{
    0x18D1, 0x04E8, 0x2717, 0x12D1, 0x339B, 0x22D9, 0x2A70, 0x2D95, 0x2A45, 0x19D2,
    0x17EF, 0x22B8, 0x1004, 0x0FCE, 0x0BB4, 0x0B05, 0x2E04, 0x1EBF, 0x0E8D, 0x05C6,
    0x1BBB, 0x2B4C, 0x29A9, 0x109B, 0x1F3A, 0x2207, 0x1782, 0x0489, 0x04DD, 0x2AE5,
};

struct Functions {
    bool adb = false;
    bool mtp = false;
    bool vendorOrComm = false;
};

struct RegistryStrings {
    std::string serial;
    std::string product;
};

bool knownAndroidVendor(std::uint16_t vendor) noexcept
{
    return std::ranges::find(kAndroidVendors, vendor) != kAndroidVendors.end();
}

bool sameSerial(std::string_view a, std::string_view b) noexcept
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

bool validSerial(std::string_view text) noexcept
{
    return !text.empty() && std::ranges::all_of(text, [](char c) {
               const auto byte = static_cast<unsigned char>(c);
               return byte > 0x20U && byte < 0x7FU;
           });
}

std::string trimUsbText(std::string text)
{
    while (!text.empty() && (text.back() == '\0' || std::isspace(static_cast<unsigned char>(text.back())))) {
        text.pop_back();
    }
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    return start == 0 ? std::move(text) : text.substr(start);
}

std::string readUsbString(libusb_device_handle* handle, std::uint8_t index)
{
    if (handle == nullptr || index == 0) {
        return {};
    }
    std::array<unsigned char, 256> raw{};
    std::uint16_t langId = 0x0409;
    if (libusb_get_string_descriptor(handle, 0, 0, raw.data(), 4) >= 4 && raw[1] == LIBUSB_DT_STRING) {
        langId = static_cast<std::uint16_t>(raw[2] | (static_cast<std::uint16_t>(raw[3]) << 8U));
    }
    const int bytes = libusb_get_string_descriptor(handle, index, langId, raw.data(), static_cast<int>(raw.size()));
    if (bytes >= 4 && raw[1] == LIBUSB_DT_STRING) {
        const std::size_t utf16Bytes = std::min<std::size_t>(static_cast<std::size_t>(bytes), raw[0]) - 2U;
        @autoreleasepool {
            NSString* decoded = [[NSString alloc] initWithBytes:raw.data() + 2
                                                         length:utf16Bytes & ~std::size_t{1}
                                                       encoding:NSUTF16LittleEndianStringEncoding];
            if (decoded.length > 0) {
                return trimUsbText(std::string(decoded.UTF8String));
            }
        }
    }
    const int asciiLen = libusb_get_string_descriptor_ascii(handle, index, raw.data(), static_cast<int>(raw.size()));
    if (asciiLen <= 0) {
        return {};
    }
    return trimUsbText(std::string(reinterpret_cast<const char*>(raw.data()), static_cast<std::size_t>(asciiLen)));
}

std::optional<std::int64_t> registryInt(io_registry_entry_t entry, CFStringRef key)
{
    CFTypeRef value = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
    if (value == nullptr) {
        return std::nullopt;
    }
    std::optional<std::int64_t> out;
    if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        long long number = 0;
        if (CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberLongLongType, &number)) {
            out = static_cast<std::int64_t>(number);
        }
    }
    CFRelease(value);
    return out;
}

std::string registryString(io_registry_entry_t entry, CFStringRef key)
{
    CFTypeRef value = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
    if (value == nullptr) {
        return {};
    }
    std::string out;
    if (CFGetTypeID(value) == CFStringGetTypeID()) {
        @autoreleasepool {
            NSString* text = (__bridge NSString*)static_cast<CFStringRef>(value);
            if (text.length > 0) {
                out = trimUsbText(std::string(text.UTF8String));
            }
        }
    }
    CFRelease(value);
    return out;
}

RegistryStrings ioKitStringsFor(std::uint16_t vendorId, std::uint16_t productId, std::uint8_t address)
{
    RegistryStrings fallback;
    for (const char* className : {"IOUSBHostDevice", "IOUSBDevice"}) {
        CFMutableDictionaryRef matching = IOServiceMatching(className);
        if (matching == nullptr) {
            continue;
        }
        io_iterator_t iterator = 0;
        if (IOServiceGetMatchingServices(kIOMainPortDefault, matching, &iterator) != KERN_SUCCESS) {
            continue;
        }
        for (io_service_t service = IOIteratorNext(iterator); service != 0; service = IOIteratorNext(iterator)) {
            const auto vid = registryInt(service, CFSTR("idVendor"));
            const auto pid = registryInt(service, CFSTR("idProduct"));
            if (vid == vendorId && pid == productId) {
                std::string serial = registryString(service, CFSTR("kUSBSerialNumberString"));
                if (serial.empty()) {
                    serial = registryString(service, CFSTR("USB Serial Number"));
                }
                std::string product = registryString(service, CFSTR("kUSBProductString"));
                if (product.empty()) {
                    product = registryString(service, CFSTR("USB Product Name"));
                }
                std::optional<std::int64_t> devAddr = registryInt(service, CFSTR("usbAddress"));
                if (!devAddr) {
                    devAddr = registryInt(service, CFSTR("USB Address"));
                }
                if (devAddr && *devAddr == address && (!serial.empty() || !product.empty())) {
                    IOObjectRelease(service);
                    IOObjectRelease(iterator);
                    return {.serial = std::move(serial), .product = std::move(product)};
                }
                if (fallback.serial.empty() && !serial.empty()) {
                    fallback.serial = std::move(serial);
                    fallback.product = std::move(product);
                }
            }
            IOObjectRelease(service);
        }
        IOObjectRelease(iterator);
    }
    return fallback;
}

Functions inspectConfig(libusb_device* device)
{
    Functions functions;
    libusb_config_descriptor* config = nullptr;
    if (libusb_get_active_config_descriptor(device, &config) != LIBUSB_SUCCESS || config == nullptr) {
        if (libusb_get_config_descriptor(device, 0, &config) != LIBUSB_SUCCESS || config == nullptr) {
            return functions;
        }
    }
    for (std::uint8_t i = 0; i < config->bNumInterfaces; ++i) {
        const libusb_interface& iface = config->interface[i];
        for (int alt = 0; alt < iface.num_altsetting; ++alt) {
            const libusb_interface_descriptor& desc = iface.altsetting[alt];
            if (desc.bInterfaceClass == 0xFF && desc.bInterfaceSubClass == 0x42 && desc.bInterfaceProtocol == 0x01) {
                functions.adb = true;
            } else if (desc.bInterfaceClass == 0x06 && desc.bInterfaceSubClass == 0x01 &&
                       desc.bInterfaceProtocol == 0x01) {
                functions.mtp = true;
            }
            if (desc.bInterfaceClass == 0xFF || desc.bInterfaceClass == 0x02 || desc.bInterfaceClass == 0x0A) {
                functions.vendorOrComm = true;
            }
        }
    }
    libusb_free_config_descriptor(config);
    return functions;
}

libusb_context* usbContext()
{
    static libusb_context* const context = [] {
        libusb_context* made = nullptr;
        return libusb_init_context(&made, nullptr, 0) == LIBUSB_SUCCESS ? made : nullptr;
    }();
    return context;
}

UsbFailure failureOf(int error) noexcept
{
    switch (error) {
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return UsbFailure::NoDriver;
    case LIBUSB_ERROR_ACCESS:
    case LIBUSB_ERROR_BUSY:
        return UsbFailure::Busy;
    case LIBUSB_ERROR_NO_DEVICE:
    case LIBUSB_ERROR_NOT_FOUND:
        return UsbFailure::NotFound;
    default:
        return UsbFailure::Refused;
    }
}

} // namespace

std::vector<UsbPhone> listUsbPhones()
{
    std::vector<UsbPhone> phones;
    libusb_context* context = usbContext();
    if (context == nullptr) {
        return phones;
    }
    libusb_device** devices = nullptr;
    const ssize_t count = libusb_get_device_list(context, &devices);
    for (ssize_t at = 0; at < count; ++at) {
        libusb_device_descriptor descriptor{};
        if (libusb_get_device_descriptor(devices[at], &descriptor) != LIBUSB_SUCCESS ||
            descriptor.bDeviceClass == LIBUSB_CLASS_HUB || descriptor.idVendor == kAppleVendor ||
            descriptor.iSerialNumber == 0) {
            continue;
        }
        const Functions functions = inspectConfig(devices[at]);
        const bool accessory = descriptor.idVendor == kGoogleVendor && descriptor.idProduct >= kFirstAccessory &&
                               descriptor.idProduct <= kLastAccessory;
        const bool androidVendorPhone = knownAndroidVendor(descriptor.idVendor) && functions.vendorOrComm;
        if (!functions.adb && !functions.mtp && !accessory && !androidVendorPhone) {
            continue;
        }

        std::string serial;
        std::string name;
        libusb_device_handle* handle = nullptr;
        if (libusb_open(devices[at], &handle) == LIBUSB_SUCCESS && handle != nullptr) {
            serial = readUsbString(handle, descriptor.iSerialNumber);
            if (descriptor.iProduct != 0) {
                name = readUsbString(handle, descriptor.iProduct);
            }
            libusb_close(handle);
        }
        if (serial.empty() || (name.empty() && descriptor.iProduct != 0)) {
            RegistryStrings fallback =
                ioKitStringsFor(descriptor.idVendor, descriptor.idProduct, libusb_get_device_address(devices[at]));
            if (serial.empty()) {
                serial = std::move(fallback.serial);
            }
            if (name.empty()) {
                name = std::move(fallback.product);
            }
        }
        if (!validSerial(serial)) {
            continue;
        }
        if (std::ranges::any_of(phones, [&](const UsbPhone& seen) { return sameSerial(seen.serial, serial); })) {
            continue;
        }
        phones.push_back({.serial = std::move(serial),
                          .name = std::move(name),
                          .vendorId = descriptor.idVendor,
                          .productId = descriptor.idProduct,
                          .driver = UsbDriver::Ready,
                          .debugging = functions.adb});
    }
    if (count >= 0) {
        libusb_free_device_list(devices, 1);
    }
    std::ranges::sort(phones, {}, &UsbPhone::serial);
    return phones;
}

std::string usbLibraryVersion()
{
    const libusb_version* version = libusb_get_version();
    return std::format("{}.{}.{}", version->major, version->minor, version->micro);
}

struct UsbDevice::Impl {
    libusb_device_handle* handle = nullptr;
    std::size_t packetSize = kDefaultPacketSize;
    UsbFailure failure = UsbFailure::None;

    Impl() = default;
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    ~Impl()
    {
        if (handle != nullptr) {
            libusb_close(handle);
        }
    }
};

UsbDevice::UsbDevice(std::unique_ptr<Impl> impl)
    : m_impl(std::move(impl))
{
}

UsbDevice::~UsbDevice() = default;

std::unique_ptr<UsbDevice> UsbDevice::open(const UsbPhone& phone, UsbFailure& failure)
{
    failure = UsbFailure::NotFound;
    if (phone.serial.empty()) {
        return nullptr;
    }
    libusb_context* context = usbContext();
    if (context == nullptr) {
        failure = UsbFailure::Refused;
        return nullptr;
    }
    libusb_device** devices = nullptr;
    const ssize_t count = libusb_get_device_list(context, &devices);
    std::unique_ptr<UsbDevice> opened;
    for (ssize_t at = 0; at < count && opened == nullptr; ++at) {
        libusb_device_descriptor descriptor{};
        if (libusb_get_device_descriptor(devices[at], &descriptor) != LIBUSB_SUCCESS ||
            descriptor.idVendor != phone.vendorId || descriptor.idProduct != phone.productId ||
            descriptor.iSerialNumber == 0) {
            continue;
        }
        libusb_device_handle* handle = nullptr;
        if (const int result = libusb_open(devices[at], &handle); result != LIBUSB_SUCCESS) {
            const UsbFailure reason = failureOf(result);
            if (reason != UsbFailure::NotFound) {
                failure = reason;
            }
            continue;
        }
        std::string serial = readUsbString(handle, descriptor.iSerialNumber);
        if (serial.empty()) {
            serial = ioKitStringsFor(descriptor.idVendor, descriptor.idProduct, libusb_get_device_address(devices[at]))
                         .serial;
        }
        if (!sameSerial(serial, phone.serial)) {
            libusb_close(handle);
            continue;
        }
        auto impl = std::make_unique<Impl>();
        impl->handle = handle;
        impl->packetSize = descriptor.bMaxPacketSize0 > 0 ? descriptor.bMaxPacketSize0 : kDefaultPacketSize;
        opened.reset(new UsbDevice(std::move(impl)));
        failure = UsbFailure::None;
    }
    if (count >= 0) {
        libusb_free_device_list(devices, 1);
    }
    return opened;
}

bool UsbDevice::controlOut(std::uint8_t request, std::uint16_t value, std::uint16_t index,
                           std::span<const std::uint8_t> data)
{
    if (m_impl->failure == UsbFailure::Gone || data.size() > 0xFFFF) {
        return false;
    }
    const int sent = libusb_control_transfer(m_impl->handle, kVendorToDevice, request, value, index,
                                             const_cast<unsigned char*>(data.data()),
                                             static_cast<std::uint16_t>(data.size()), kTransferTimeoutMs);
    if (sent == LIBUSB_ERROR_NO_DEVICE) {
        m_impl->failure = UsbFailure::Gone;
        return false;
    }
    if (sent < 0 || static_cast<std::size_t>(sent) != data.size()) {
        m_impl->failure = UsbFailure::Refused;
        return false;
    }
    m_impl->failure = UsbFailure::None;
    return true;
}

std::optional<std::vector<std::uint8_t>> UsbDevice::controlIn(std::uint8_t request, std::uint16_t value,
                                                              std::uint16_t index, std::size_t length)
{
    if (m_impl->failure == UsbFailure::Gone || length > 0xFFFF) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> data(length, 0);
    const int received = libusb_control_transfer(m_impl->handle, kVendorFromDevice, request, value, index,
                                                 data.data(), static_cast<std::uint16_t>(data.size()),
                                                 kTransferTimeoutMs);
    if (received == LIBUSB_ERROR_NO_DEVICE) {
        m_impl->failure = UsbFailure::Gone;
        return std::nullopt;
    }
    if (received < 0) {
        m_impl->failure = UsbFailure::Refused;
        return std::nullopt;
    }
    m_impl->failure = UsbFailure::None;
    data.resize(static_cast<std::size_t>(received));
    return data;
}

std::size_t UsbDevice::maxPacketSize() const noexcept
{
    return m_impl->packetSize;
}

UsbFailure UsbDevice::failure() const noexcept
{
    return m_impl->failure;
}

struct UsbWatcher::Impl {
    Changed changed;
    libusb_context* context = nullptr;
    libusb_hotplug_callback_handle callback = 0;
    std::atomic<bool> stopping{false};
    bool active = false;
    std::thread worker;

    static int LIBUSB_CALL onHotplug(libusb_context*, libusb_device*, libusb_hotplug_event, void* userData)
    {
        const auto* impl = static_cast<const Impl*>(userData);
        if (!impl->stopping.load(std::memory_order_acquire) && impl->changed) {
            impl->changed();
        }
        return 0;
    }

    void run() const
    {
        setCurrentThreadName("usb watch");
        while (!stopping.load(std::memory_order_acquire)) {
            timeval timeout{.tv_sec = 60, .tv_usec = 0};
            libusb_handle_events_timeout_completed(context, &timeout, nullptr);
        }
    }
};

UsbWatcher::UsbWatcher(Changed changed)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->changed = std::move(changed);
    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) == 0 ||
        libusb_init_context(&m_impl->context, nullptr, 0) != LIBUSB_SUCCESS || m_impl->context == nullptr) {
        return;
    }
    const auto events = static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                                          LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT);
    if (libusb_hotplug_register_callback(m_impl->context, events, LIBUSB_HOTPLUG_NO_FLAGS, LIBUSB_HOTPLUG_MATCH_ANY,
                                         LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, &Impl::onHotplug,
                                         m_impl.get(), &m_impl->callback) != LIBUSB_SUCCESS) {
        libusb_exit(m_impl->context);
        m_impl->context = nullptr;
        return;
    }
    m_impl->active = true;
    m_impl->worker = std::thread([impl = m_impl.get()] { impl->run(); });
}

UsbWatcher::~UsbWatcher()
{
    if (m_impl->context == nullptr) {
        return;
    }
    m_impl->stopping.store(true, std::memory_order_release);
    if (m_impl->active) {
        libusb_hotplug_deregister_callback(m_impl->context, m_impl->callback);
    }
    libusb_interrupt_event_handler(m_impl->context);
    if (m_impl->worker.joinable()) {
        m_impl->worker.join();
    }
    libusb_exit(m_impl->context);
}

bool UsbWatcher::started() const noexcept
{
    return m_impl->active;
}

} // namespace gs::platform

#else

namespace gs::platform {

struct UsbDevice::Impl {};
struct UsbWatcher::Impl {};

std::vector<UsbPhone> listUsbPhones()
{
    return {};
}

std::string usbLibraryVersion()
{
    return "none";
}

UsbDevice::UsbDevice(std::unique_ptr<Impl> impl)
    : m_impl(std::move(impl))
{
}

UsbDevice::~UsbDevice() = default;

std::unique_ptr<UsbDevice> UsbDevice::open(const UsbPhone&, UsbFailure& failure)
{
    failure = UsbFailure::NotFound;
    return nullptr;
}

bool UsbDevice::controlOut(std::uint8_t, std::uint16_t, std::uint16_t, std::span<const std::uint8_t>)
{
    return false;
}

std::optional<std::vector<std::uint8_t>> UsbDevice::controlIn(std::uint8_t, std::uint16_t, std::uint16_t, std::size_t)
{
    return std::nullopt;
}

std::size_t UsbDevice::maxPacketSize() const noexcept
{
    return 0;
}

UsbFailure UsbDevice::failure() const noexcept
{
    return UsbFailure::NotFound;
}

UsbWatcher::UsbWatcher(Changed)
    : m_impl(std::make_unique<Impl>())
{
}

UsbWatcher::~UsbWatcher() = default;

bool UsbWatcher::started() const noexcept
{
    return false;
}

} // namespace gs::platform

#endif
