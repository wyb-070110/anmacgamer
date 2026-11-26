// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Serial port discovery with IOKit (/dev/cu.*) and non-blocking termios transport on macOS.
#include "Worker.h"

#include "gs/hardware/Backend.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace gs::hardware {

namespace {

constexpr std::size_t kReadChunk = 4096;
constexpr int kWriteTimeoutMs = 2000;
constexpr cc_t kXon = 0x11;
constexpr cc_t kXoff = 0x13;
constexpr std::string_view kSerialIdPrefix = "serial:";
constexpr std::string_view kCalloutPrefix = "/dev/cu.";

std::string trimText(std::string text)
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

std::string cfStringToUtf8(CFStringRef value)
{
    if (value == nullptr) {
        return {};
    }
    const CFIndex length = CFStringGetLength(value);
    const CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    if (maxSize <= 1) {
        return {};
    }
    std::string buffer(static_cast<std::size_t>(maxSize), '\0');
    if (!CFStringGetCString(value, buffer.data(), maxSize, kCFStringEncodingUTF8)) {
        return {};
    }
    buffer.resize(std::strlen(buffer.c_str()));
    return trimText(std::move(buffer));
}

std::string registryString(io_registry_entry_t entry, CFStringRef key)
{
    CFTypeRef prop = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
    if (prop == nullptr) {
        return {};
    }
    std::string out;
    if (CFGetTypeID(prop) == CFStringGetTypeID()) {
        out = cfStringToUtf8(static_cast<CFStringRef>(prop));
    }
    CFRelease(prop);
    return out;
}

std::optional<std::uint16_t> registryUint16(io_registry_entry_t entry, CFStringRef key)
{
    CFTypeRef prop = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
    if (prop == nullptr) {
        return std::nullopt;
    }
    std::optional<std::uint16_t> out;
    if (CFGetTypeID(prop) == CFNumberGetTypeID()) {
        long long number = 0;
        if (CFNumberGetValue(static_cast<CFNumberRef>(prop), kCFNumberLongLongType, &number) && number >= 0 &&
            number <= 0xFFFF) {
            out = static_cast<std::uint16_t>(number);
        }
    }
    CFRelease(prop);
    return out;
}

struct UsbMetadata {
    std::uint16_t vendorId = 0;
    std::uint16_t productId = 0;
    std::string productName;
};

UsbMetadata usbMetadataOf(io_registry_entry_t serialEntry)
{
    UsbMetadata meta;
    io_registry_entry_t current = serialEntry;
    IOObjectRetain(current);
    for (int depth = 0; depth < 16; ++depth) {
        io_registry_entry_t parent = 0;
        const kern_return_t kr = IORegistryEntryGetParentEntry(current, kIOServicePlane, &parent);
        IOObjectRelease(current);
        if (kr != KERN_SUCCESS || parent == 0) {
            break;
        }
        current = parent;

        if (meta.vendorId == 0 && meta.productId == 0) {
            const auto vid = registryUint16(current, CFSTR("idVendor"));
            const auto pid = registryUint16(current, CFSTR("idProduct"));
            if (vid && pid) {
                meta.vendorId = *vid;
                meta.productId = *pid;
            }
        }
        if (meta.productName.empty()) {
            meta.productName = registryString(current, CFSTR("USB Product Name"));
            if (meta.productName.empty()) {
                meta.productName = registryString(current, CFSTR("kUSBProductString"));
            }
        }
        if ((meta.vendorId != 0 || meta.productId != 0) && !meta.productName.empty()) {
            IOObjectRelease(current);
            break;
        }
    }
    return meta;
}

std::vector<DeviceInfo> listMacSerialPorts()
{
    std::vector<DeviceInfo> ports;
    CFMutableDictionaryRef matching = IOServiceMatching(kIOSerialBSDServiceValue);
    if (matching == nullptr) {
        return ports;
    }
    io_iterator_t iterator = 0;
    if (IOServiceGetMatchingServices(kIOMainPortDefault, matching, &iterator) != KERN_SUCCESS) {
        return ports;
    }
    for (io_service_t service = IOIteratorNext(iterator); service != 0; service = IOIteratorNext(iterator)) {
        std::string path = registryString(service, CFSTR(kIOCalloutDeviceKey));
        if (path.starts_with(kCalloutPrefix)) {
            const UsbMetadata usb = usbMetadataOf(service);
            std::string name = usb.productName;
            if (name.empty()) {
                name = registryString(service, CFSTR(kIOTTYBaseNameKey));
            }
            if (name.empty()) {
                name = path.substr(std::string_view("/dev/").size());
            }
            std::string id = std::string(kSerialIdPrefix) + path;
            if (std::ranges::none_of(ports, [&id](const DeviceInfo& seen) { return seen.id == id; })) {
                DeviceInfo info;
                info.id = std::move(id);
                info.kind = Kind::Serial;
                info.name = std::move(name);
                info.path = std::move(path);
                info.vendorId = usb.vendorId;
                info.productId = usb.productId;
                ports.push_back(std::move(info));
            }
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);
    std::ranges::sort(ports, {}, &DeviceInfo::id);
    return ports;
}

std::string systemErrorMessage(int err)
{
    if (err == 0) {
        return "serial error";
    }
    std::array<char, 256> buffer{};
    if (::strerror_r(err, buffer.data(), buffer.size()) == 0 && buffer[0] != '\0') {
        return std::string(buffer.data());
    }
    return "error " + std::to_string(err);
}

bool removalErrno(int err) noexcept
{
    switch (err) {
    case ENXIO:
    case EIO:
    case ENODEV:
    case ENOENT:
    case ECONNRESET:
    case EPIPE:
    case EBADF:
        return true;
    default:
        return false;
    }
}

std::string applySignalsNow(int fd, std::optional<bool> dtr, std::optional<bool> rts, bool ignoreUnsupported)
{
    auto setBit = [&](int bit, bool on) -> std::string {
        int mask = bit;
        if (::ioctl(fd, on ? TIOCMBIS : TIOCMBIC, &mask) == 0) {
            return {};
        }
        int err = errno;
        if (bit == TIOCM_DTR && ::ioctl(fd, on ? TIOCSDTR : TIOCCDTR) == 0) {
            return {};
        }
        if (ignoreUnsupported && (err == ENOTTY || err == ENODEV || err == EINVAL)) {
            return {};
        }
        return systemErrorMessage(err);
    };
    if (dtr) {
        if (std::string problem = setBit(TIOCM_DTR, *dtr); !problem.empty()) {
            return problem;
        }
    }
    if (rts) {
        if (std::string problem = setBit(TIOCM_RTS, *rts); !problem.empty()) {
            return problem;
        }
    }
    return {};
}

std::string configureTermios(int fd, const SerialOptions& options)
{
    if (options.stopBits == StopBits::OneAndHalf) {
        return "1.5 stop bits are not supported on macOS";
    }
    if (options.parity == Parity::Mark || options.parity == Parity::Space) {
        return "mark and space parity are not supported on macOS";
    }

    termios tio{};
    if (::tcgetattr(fd, &tio) != 0) {
        return systemErrorMessage(errno);
    }
    ::cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);

    tio.c_cflag &= ~static_cast<tcflag_t>(CSIZE);
    switch (options.dataBits) {
    case 5:
        tio.c_cflag |= CS5;
        break;
    case 6:
        tio.c_cflag |= CS6;
        break;
    case 7:
        tio.c_cflag |= CS7;
        break;
    case 8:
    default:
        tio.c_cflag |= CS8;
        break;
    }

    if (options.stopBits == StopBits::Two) {
        tio.c_cflag |= CSTOPB;
    } else {
        tio.c_cflag &= ~static_cast<tcflag_t>(CSTOPB);
    }

    tio.c_cflag &= ~static_cast<tcflag_t>(PARENB | PARODD);
    tio.c_iflag &= ~static_cast<tcflag_t>(INPCK);
    if (options.parity == Parity::Even) {
        tio.c_cflag |= PARENB;
        tio.c_iflag |= INPCK;
    } else if (options.parity == Parity::Odd) {
        tio.c_cflag |= (PARENB | PARODD);
        tio.c_iflag |= INPCK;
    }

    tio.c_cflag &= ~static_cast<tcflag_t>(CRTSCTS);
    tio.c_iflag &= ~static_cast<tcflag_t>(IXON | IXOFF | IXANY);
    if (options.flowControl == FlowControl::Hardware) {
        tio.c_cflag |= CRTSCTS;
    } else if (options.flowControl == FlowControl::Software) {
        tio.c_iflag |= (IXON | IXOFF);
        tio.c_cc[VSTART] = kXon;
        tio.c_cc[VSTOP] = kXoff;
    }

    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    const auto desiredSpeed = static_cast<speed_t>(options.baudRate);
    if (::cfsetspeed(&tio, desiredSpeed) == 0 && ::tcsetattr(fd, TCSANOW, &tio) == 0) {
        return {};
    }
    ::cfsetspeed(&tio, B115200);
    if (::tcsetattr(fd, TCSANOW, &tio) != 0) {
        return systemErrorMessage(errno);
    }
    speed_t customSpeed = desiredSpeed;
    if (::ioctl(fd, IOSSIOSPEED, &customSpeed) != 0) {
        return systemErrorMessage(errno);
    }
    return {};
}

class PosixSerialLink final : public Device
{
public:
    PosixSerialLink(int fd, std::shared_ptr<Events> events)
        : m_fd(fd)
        , m_events(std::move(events))
    {
        if (::pipe(m_wakePipe.data()) == 0) {
            for (const int pipeFd : m_wakePipe) {
                const int flags = ::fcntl(pipeFd, F_GETFL, 0);
                if (flags >= 0) {
                    static_cast<void>(::fcntl(pipeFd, F_SETFL, flags | O_NONBLOCK));
                }
                static_cast<void>(::fcntl(pipeFd, F_SETFD, FD_CLOEXEC));
            }
            m_ready = true;
            m_reader = std::thread([this] { readLoop(); });
        }
    }

    ~PosixSerialLink() override
    {
        m_stopping.store(true, std::memory_order_release);
        if (m_wakePipe[1] >= 0) {
            const std::uint8_t byte = 1;
            static_cast<void>(::write(m_wakePipe[1], &byte, 1));
        }
        if (m_reader.joinable()) {
            m_reader.join();
        }
        m_writer.stop();
        if (m_fd >= 0) {
            ::close(m_fd);
        }
        for (const int pipeFd : m_wakePipe) {
            if (pipeFd >= 0) {
                ::close(pipeFd);
            }
        }
    }

    PosixSerialLink(const PosixSerialLink&) = delete;
    PosixSerialLink& operator=(const PosixSerialLink&) = delete;

    [[nodiscard]] bool ready() const noexcept { return m_ready; }

    void write(Bytes bytes, Completion done) override
    {
        m_writer.post([this, bytes = std::move(bytes), done = std::move(done)] { done(writeNow(bytes)); });
    }

    void setSignals(std::optional<bool> dtr, std::optional<bool> rts, Completion done) override
    {
        m_writer.post([this, dtr, rts, done = std::move(done)] {
            if (m_stopping.load(std::memory_order_acquire) || m_closed.load(std::memory_order_acquire)) {
                done("the device was closed");
                return;
            }
            done(applySignalsNow(m_fd, dtr, rts, false));
        });
    }

private:
    std::string writeNow(const Bytes& bytes)
    {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            if (m_stopping.load(std::memory_order_acquire) || m_closed.load(std::memory_order_acquire)) {
                return "the device was closed";
            }
            const std::size_t chunk = std::min<std::size_t>(bytes.size() - offset, kMostWriteBytes);
            const ssize_t written = ::write(m_fd, bytes.data() + offset, chunk);
            if (written > 0) {
                offset += static_cast<std::size_t>(written);
                continue;
            }
            const int err = written < 0 ? errno : EAGAIN;
            if (err == EINTR) {
                continue;
            }
            if (err != EAGAIN && err != EWOULDBLOCK) {
                return systemErrorMessage(err);
            }
            std::array<pollfd, 2> fds{{
                {.fd = m_fd, .events = POLLOUT, .revents = 0},
                {.fd = m_wakePipe[0], .events = POLLIN, .revents = 0},
            }};
            int polled = 0;
            do {
                polled = ::poll(fds.data(), static_cast<nfds_t>(fds.size()), kWriteTimeoutMs);
            } while (polled < 0 && errno == EINTR);
            if (polled == 0) {
                return "the device did not take the data in time";
            }
            if (polled < 0) {
                return systemErrorMessage(errno);
            }
            if ((fds[1].revents & POLLIN) != 0 || m_stopping.load(std::memory_order_acquire)) {
                return "the device was closed";
            }
            if ((fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                return "the device is gone";
            }
        }
        return {};
    }

    void readLoop()
    {
        pthread_setname_np("serial read");
        std::vector<std::uint8_t> buffer(kReadChunk);
        for (;;) {
            std::array<pollfd, 2> fds{{
                {.fd = m_fd, .events = POLLIN, .revents = 0},
                {.fd = m_wakePipe[0], .events = POLLIN, .revents = 0},
            }};
            const int polled = ::poll(fds.data(), static_cast<nfds_t>(fds.size()), -1);
            if (polled < 0) {
                if (errno == EINTR) {
                    continue;
                }
                lost(errno);
                return;
            }
            if ((fds[1].revents & POLLIN) != 0 || m_stopping.load(std::memory_order_acquire)) {
                return;
            }
            if ((fds[0].revents & POLLIN) != 0) {
                for (;;) {
                    const ssize_t received = ::read(m_fd, buffer.data(), buffer.size());
                    if (received > 0) {
                        if (m_stopping.load(std::memory_order_acquire)) {
                            return;
                        }
                        if (m_events != nullptr) {
                            m_events->data(Bytes(buffer.begin(), buffer.begin() + received));
                        }
                        continue;
                    }
                    if (received == 0) {
                        lost(ENXIO);
                        return;
                    }
                    const int err = errno;
                    if (err == EINTR) {
                        continue;
                    }
                    if (err == EAGAIN || err == EWOULDBLOCK) {
                        break;
                    }
                    lost(err);
                    return;
                }
            }
            if ((fds[0].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0) {
                lost(ENXIO);
                return;
            }
        }
    }

    void lost(int err)
    {
        if (m_stopping.load(std::memory_order_acquire) || m_closed.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        if (m_events != nullptr) {
            const Closed reason = removalErrno(err) ? Closed::Removed : Closed::Failed;
            const std::string problem = err == ENXIO ? "the device is gone" : systemErrorMessage(err);
            m_events->closed(reason, problem);
        }
    }

    int m_fd = -1;
    std::shared_ptr<Events> m_events;
    std::array<int, 2> m_wakePipe{{-1, -1}};
    bool m_ready = false;
    std::atomic<bool> m_stopping{false};
    std::atomic<bool> m_closed{false};
    detail::Worker m_writer;
    std::thread m_reader;
};

std::string resolveSerialPath(const DeviceInfo& device)
{
    if (!device.path.empty()) {
        return device.path;
    }
    if (device.id.starts_with(kSerialIdPrefix)) {
        return device.id.substr(kSerialIdPrefix.size());
    }
    return {};
}

OpenResult openMacSerial(const DeviceInfo& device, const SerialOptions& options, std::shared_ptr<Events> events)
{
    OpenResult result;
    if (!validSerialOptions(options)) {
        result.problem = "the serial settings are not possible";
        return result;
    }
    const std::string path = resolveSerialPath(device);
    if (!path.starts_with("/dev/")) {
        result.missing = true;
        result.problem = "the serial device was not found";
        return result;
    }
    const int fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        const int err = errno;
        result.missing = (err == ENOENT || err == ENXIO || err == ENODEV);
        result.problem = systemErrorMessage(err);
        return result;
    }
    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        const int err = errno;
        ::close(fd);
        result.problem = err == EWOULDBLOCK ? "the serial port is already in use" : systemErrorMessage(err);
        return result;
    }
    if (::ioctl(fd, TIOCEXCL) != 0 && errno == EBUSY) {
        const int err = errno;
        ::close(fd);
        result.problem = systemErrorMessage(err);
        return result;
    }
    if (std::string problem = configureTermios(fd, options); !problem.empty()) {
        ::close(fd);
        result.problem = std::move(problem);
        return result;
    }
    if (const int flags = ::fcntl(fd, F_GETFL, 0); flags >= 0) {
        static_cast<void>(::fcntl(fd, F_SETFL, flags | O_NONBLOCK));
    }
    static_cast<void>(::tcflush(fd, TCIOFLUSH));
    const std::optional<bool> initialRts =
        options.flowControl == FlowControl::Hardware ? std::nullopt : std::optional<bool>(options.rts);
    static_cast<void>(applySignalsNow(fd, options.dtr, initialRts, true));

    auto link = std::make_unique<PosixSerialLink>(fd, std::move(events));
    if (!link->ready()) {
        result.problem = "the device could not be set up";
        return result;
    }
    result.device = std::move(link);
    return result;
}

void drainIterator(io_iterator_t iterator)
{
    for (io_service_t service = IOIteratorNext(iterator); service != 0; service = IOIteratorNext(iterator)) {
        IOObjectRelease(service);
    }
}

struct SharedState {
    mutable std::mutex mutex;
    std::vector<DeviceInfo> wired;
    std::function<void()> changed;

    void notify() const
    {
        std::function<void()> callback;
        {
            const std::scoped_lock lock(mutex);
            callback = changed;
        }
        if (callback) {
            callback();
        }
    }
};

class MacSerialBackend final : public Backend
{
public:
    MacSerialBackend()
        : m_shared(std::make_shared<SharedState>())
        , m_control(std::make_shared<detail::Worker>())
    {
        m_shared->wired = listMacSerialPorts();
        startHotplugThread();
    }

    ~MacSerialBackend() override
    {
        stopHotplugThread();
        m_control->stop();
    }

    MacSerialBackend(const MacSerialBackend&) = delete;
    MacSerialBackend& operator=(const MacSerialBackend&) = delete;

    [[nodiscard]] std::vector<DeviceInfo> devices() const override
    {
        const std::scoped_lock lock(m_shared->mutex);
        return m_shared->wired;
    }

    void watch(std::function<void()> changed) override
    {
        {
            const std::scoped_lock lock(m_shared->mutex);
            m_shared->changed = std::move(changed);
        }
        requestRefresh();
    }

    void scan(bool) override {}

    void open(OpenRequest request, Opened done) override
    {
        m_control->post([request = std::move(request), done = std::move(done)] {
            if (!done) {
                return;
            }
            if (request.device.kind != Kind::Serial) {
                OpenResult unsupported;
                unsupported.missing = true;
                unsupported.problem = "HID and Bluetooth LE devices are not available on macOS yet";
                done(std::move(unsupported));
                return;
            }
            done(openMacSerial(request.device, request.serial, request.events));
        });
    }

private:
    static void onServicesChanged(void* refcon, io_iterator_t iterator)
    {
        drainIterator(iterator);
        auto* self = static_cast<MacSerialBackend*>(refcon);
        if (!self->m_stoppingHotplug.load(std::memory_order_acquire)) {
            self->requestRefresh();
        }
    }

    void requestRefresh()
    {
        if (!m_refreshWanted.exchange(true, std::memory_order_acq_rel)) {
            m_control->post([this] { refresh(); });
        }
    }

    void refresh()
    {
        m_refreshWanted.store(false, std::memory_order_release);
        std::vector<DeviceInfo> wired = listMacSerialPorts();
        bool fresh = false;
        {
            const std::scoped_lock lock(m_shared->mutex);
            if (wired != m_shared->wired) {
                m_shared->wired = std::move(wired);
                fresh = true;
            }
        }
        if (fresh) {
            m_shared->notify();
        }
    }

    void startHotplugThread()
    {
        std::mutex readyMutex;
        std::condition_variable readyCv;
        bool initialized = false;
        m_hotplugThread = std::thread([this, &readyMutex, &readyCv, &initialized] {
            pthread_setname_np("serial watch");
            IONotificationPortRef port = IONotificationPortCreate(kIOMainPortDefault);
            io_iterator_t added = 0;
            io_iterator_t removed = 0;
            CFRunLoopRef loop = CFRunLoopGetCurrent();
            CFRetain(loop);
            if (port != nullptr) {
                CFRunLoopAddSource(loop, IONotificationPortGetRunLoopSource(port), kCFRunLoopDefaultMode);
                if (CFMutableDictionaryRef matchAdd = IOServiceMatching(kIOSerialBSDServiceValue)) {
                    if (IOServiceAddMatchingNotification(port, kIOFirstMatchNotification, matchAdd,
                                                         &MacSerialBackend::onServicesChanged, this,
                                                         &added) == KERN_SUCCESS) {
                        drainIterator(added);
                    }
                }
                if (CFMutableDictionaryRef matchRemove = IOServiceMatching(kIOSerialBSDServiceValue)) {
                    if (IOServiceAddMatchingNotification(port, kIOTerminatedNotification, matchRemove,
                                                         &MacSerialBackend::onServicesChanged, this,
                                                         &removed) == KERN_SUCCESS) {
                        drainIterator(removed);
                    }
                }
            }
            {
                const std::scoped_lock lock(readyMutex);
                m_hotplugLoop = loop;
                initialized = true;
            }
            readyCv.notify_one();

            if (!m_stoppingHotplug.load(std::memory_order_acquire) && port != nullptr) {
                CFRunLoopRun();
            }

            if (added != 0) {
                IOObjectRelease(added);
            }
            if (removed != 0) {
                IOObjectRelease(removed);
            }
            if (port != nullptr) {
                IONotificationPortDestroy(port);
            }
            CFRelease(loop);
        });
        std::unique_lock lock(readyMutex);
        readyCv.wait(lock, [&initialized] { return initialized; });
    }

    void stopHotplugThread()
    {
        m_stoppingHotplug.store(true, std::memory_order_release);
        if (m_hotplugLoop != nullptr) {
            CFRunLoopStop(m_hotplugLoop);
        }
        if (m_hotplugThread.joinable()) {
            m_hotplugThread.join();
        }
        m_hotplugLoop = nullptr;
    }

    std::shared_ptr<SharedState> m_shared;
    std::shared_ptr<detail::Worker> m_control;
    std::atomic<bool> m_refreshWanted{false};
    std::atomic<bool> m_stoppingHotplug{false};
    CFRunLoopRef m_hotplugLoop = nullptr;
    std::thread m_hotplugThread;
};

} // namespace

std::unique_ptr<Backend> makeWindowsBackend()
{
    return std::make_unique<MacSerialBackend>();
}

} // namespace gs::hardware
