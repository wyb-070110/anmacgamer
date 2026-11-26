// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// macOS IOKit serial port enumeration, termios configuration, pty read/write/signals and hangup handling.
#include <gs/hardware/Backend.h>
#include <gs/hardware/Devices.h>

#include <doctest/doctest.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <util.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace gs::hardware;

struct Waiter {
    std::mutex mutex;
    std::condition_variable cv;

    template <typename Pred>
    bool until(Pred pred)
    {
        std::unique_lock lock(mutex);
        return cv.wait_for(lock, std::chrono::seconds(5), pred);
    }

    template <typename Mutation>
    void note(Mutation fn)
    {
        {
            const std::scoped_lock lock(mutex);
            fn();
        }
        cv.notify_all();
    }
};

class AsyncEvents final : public Events
{
public:
    explicit AsyncEvents(Waiter& waiter)
        : m_waiter(waiter)
    {
    }

    void data(Bytes bytes) override
    {
        m_waiter.note([&] { received.insert(received.end(), bytes.begin(), bytes.end()); });
    }

    void report(std::uint8_t, Bytes) override {}

    void notified(const std::string&, const std::string&, Bytes) override {}

    void closed(Closed reason, const std::string& problem) override
    {
        m_waiter.note([&] { closings.emplace_back(reason, problem); });
    }

    Bytes received;
    std::vector<std::pair<Closed, std::string>> closings;

private:
    Waiter& m_waiter;
};

OpenResult openSync(Backend& backend, OpenRequest request)
{
    Waiter waiter;
    std::optional<OpenResult> out;
    backend.open(std::move(request), [&](OpenResult result) { waiter.note([&] { out = std::move(result); }); });
    REQUIRE(waiter.until([&] { return out.has_value(); }));
    return std::move(*out);
}

std::string completeSync(const std::function<void(Completion)>& call)
{
    Waiter waiter;
    std::optional<std::string> problem;
    call([&](std::string text) { waiter.note([&] { problem = std::move(text); }); });
    REQUIRE(waiter.until([&] { return problem.has_value(); }));
    return *problem;
}

struct PtyPair {
    int master = -1;
    int slave = -1;
    std::string slavePath;

    PtyPair()
    {
        std::array<char, 256> name{};
        if (::openpty(&master, &slave, name.data(), nullptr, nullptr) == 0) {
            slavePath = name.data();
            const int flags = ::fcntl(master, F_GETFL, 0);
            if (flags >= 0) {
                static_cast<void>(::fcntl(master, F_SETFL, flags | O_NONBLOCK));
            }
        }
    }

    ~PtyPair()
    {
        if (slave >= 0) {
            ::close(slave);
        }
        if (master >= 0) {
            ::close(master);
        }
    }

    PtyPair(const PtyPair&) = delete;
    PtyPair& operator=(const PtyPair&) = delete;
};

Bytes readFromFd(int fd, std::size_t expected)
{
    Bytes out;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (out.size() < expected && std::chrono::steady_clock::now() < deadline) {
        const auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     deadline - std::chrono::steady_clock::now())
                                     .count();
        pollfd pfd{.fd = fd, .events = POLLIN, .revents = 0};
        const int ready = ::poll(&pfd, 1, static_cast<int>(std::max<long long>(1, remainingMs)));
        if (ready <= 0) {
            continue;
        }
        std::array<std::uint8_t, 256> buf{};
        const ssize_t got = ::read(fd, buf.data(), std::min<std::size_t>(buf.size(), expected - out.size()));
        if (got > 0) {
            out.insert(out.end(), buf.begin(), buf.begin() + got);
        } else if (got == 0 || (errno != EAGAIN && errno != EINTR)) {
            break;
        }
    }
    return out;
}

} // namespace

TEST_CASE("macOS hardware backend lists /dev/cu.* serial ports in sorted order and watches hotplug cleanly")
{
    for (int cycle = 0; cycle < 3; ++cycle) {
        const std::unique_ptr<Backend> backend = makeWindowsBackend();
        REQUIRE(backend != nullptr);
        const std::vector<DeviceInfo> ports = backend->devices();
        if (cycle == 0) {
            MESSAGE("IOKit /dev/cu.* serial ports: ", ports.size());
            for (const DeviceInfo& port : ports) {
                MESSAGE(std::format("  id={} name=\"{}\" path={} vid={:04x} pid={:04x}", port.id, port.name, port.path,
                                    port.vendorId, port.productId));
            }
        }
        for (const DeviceInfo& port : ports) {
            CHECK(port.kind == Kind::Serial);
            CHECK(port.path.starts_with("/dev/cu."));
            CHECK(port.id == "serial:" + port.path);
            CHECK_FALSE(port.name.empty());
        }
        CHECK(std::ranges::is_sorted(ports, {}, &DeviceInfo::id));

        backend->watch([] {});
        backend->scan(true);
        backend->scan(false);
    }
}

TEST_CASE("macOS hardware backend rejects invalid serial options, missing ports and unsupported kinds")
{
    const std::unique_ptr<Backend> backend = makeWindowsBackend();
    REQUIRE(backend != nullptr);

    DeviceInfo missingPort;
    missingPort.id = "serial:/dev/cu.gs-nonexistent-serial-0000";
    missingPort.kind = Kind::Serial;
    missingPort.path = "/dev/cu.gs-nonexistent-serial-0000";

    const OpenResult missing = openSync(*backend, {missingPort, {}, nullptr});
    CHECK(missing.device == nullptr);
    CHECK(missing.missing);
    CHECK_FALSE(missing.problem.empty());

    const OpenResult badBaud = openSync(*backend, {missingPort, SerialOptions{.baudRate = 10}, nullptr});
    CHECK(badBaud.device == nullptr);
    CHECK_FALSE(badBaud.missing);
    CHECK(badBaud.problem == "the serial settings are not possible");

    DeviceInfo hidDevice;
    hidDevice.id = "hid:test";
    hidDevice.kind = Kind::Hid;
    const OpenResult unsupported = openSync(*backend, {hidDevice, {}, nullptr});
    CHECK(unsupported.device == nullptr);
    CHECK(unsupported.missing);
    CHECK_FALSE(unsupported.problem.empty());
}

TEST_CASE("macOS serial link configures termios on a pty, transfers bytes both ways, sets DTR/RTS and reports hangup")
{
    PtyPair pty;
    REQUIRE(pty.master >= 0);
    REQUIRE(pty.slave >= 0);
    REQUIRE_FALSE(pty.slavePath.empty());

    const std::unique_ptr<Backend> backend = makeWindowsBackend();
    REQUIRE(backend != nullptr);

    Waiter waiter;
    const auto events = std::make_shared<AsyncEvents>(waiter);

    DeviceInfo ptyDevice;
    ptyDevice.id = "serial:" + pty.slavePath;
    ptyDevice.kind = Kind::Serial;
    ptyDevice.name = "Simulated Touch Box";
    ptyDevice.path = pty.slavePath;

    const SerialOptions options{
        .baudRate = 115200,
        .dataBits = 8,
        .parity = Parity::Even,
        .stopBits = StopBits::Two,
        .flowControl = FlowControl::Software,
        .dtr = true,
        .rts = false,
    };

    OpenResult opened = openSync(*backend, {ptyDevice, options, events});
    INFO(opened.problem);
    REQUIRE(opened.device != nullptr);

    // Inspect termios on the slave side before closing our extra slave descriptor.
    termios configured{};
    REQUIRE(::tcgetattr(pty.slave, &configured) == 0);
    CHECK((configured.c_cflag & CSIZE) == CS8);
    CHECK((configured.c_cflag & CSTOPB) != 0);
    CHECK((configured.c_cflag & PARENB) != 0);
    CHECK((configured.c_cflag & PARODD) == 0);
    CHECK((configured.c_iflag & (IXON | IXOFF)) == (IXON | IXOFF));

    // Exclusive lock prevents a second open of the same port while held.
    const OpenResult secondOpen = openSync(*backend, {ptyDevice, {}, nullptr});
    CHECK(secondOpen.device == nullptr);
    CHECK_FALSE(secondOpen.missing);
    CHECK_FALSE(secondOpen.problem.empty());

    // Close our duplicate slave fd so closing master later triggers immediate hangup on PosixSerialLink's fd.
    ::close(pty.slave);
    pty.slave = -1;

    // Host -> Touch box write
    const Bytes outbound{0x5A, 0xA5, 0x01, 0x02, 0x03, 0xFE};
    CHECK(completeSync([&](Completion done) { opened.device->write(outbound, std::move(done)); }).empty());
    CHECK(readFromFd(pty.master, outbound.size()) == outbound);

    // Touch box -> Host read
    const Bytes inbound{0x4F, 0x4B, 0x10, 0x20, 0x30};
    REQUIRE(::write(pty.master, inbound.data(), inbound.size()) == static_cast<ssize_t>(inbound.size()));
    REQUIRE(waiter.until([&] { return events->received.size() == inbound.size(); }));
    CHECK(events->received == inbound);

    // Modem signal calls: no-op succeeds; on a virtual pty without hardware modem pins, kernel ENOTTY is surfaced cleanly.
    CHECK(completeSync([&](Completion done) { opened.device->setSignals(std::nullopt, std::nullopt, std::move(done)); })
              .empty());
    const std::string sigProblem =
        completeSync([&](Completion done) { opened.device->setSignals(false, true, std::move(done)); });
    CHECK((sigProblem.empty() || sigProblem.find("ioctl") != std::string::npos));

    // Unsupported parity/stop-bits are rejected cleanly on open
    PtyPair pty2;
    REQUIRE(pty2.master >= 0);
    DeviceInfo ptyDevice2{.id = "serial:" + pty2.slavePath, .kind = Kind::Serial, .path = pty2.slavePath};
    const OpenResult markParity =
        openSync(*backend, {ptyDevice2, SerialOptions{.parity = Parity::Mark}, nullptr});
    CHECK(markParity.device == nullptr);
    CHECK_FALSE(markParity.problem.empty());

    // Simulate unplugging the serial adapter by closing the pty master.
    ::close(pty.master);
    pty.master = -1;
    REQUIRE(waiter.until([&] { return !events->closings.empty(); }));
    REQUIRE(events->closings.size() == 1);
    CHECK(events->closings[0].first == Closed::Removed);
    CHECK_FALSE(events->closings[0].second.empty());

    // Writes and signal changes after removal fail cleanly.
    CHECK_FALSE(completeSync([&](Completion done) { opened.device->write({0x00}, std::move(done)); }).empty());
    CHECK(completeSync([&](Completion done) { opened.device->setSignals(true, false, std::move(done)); }) ==
          "the device was closed");
}
