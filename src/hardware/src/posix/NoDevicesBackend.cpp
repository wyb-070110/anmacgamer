// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The system backend off Windows: no serial, HID or Bluetooth devices yet (IOKit and Core Bluetooth would
// list them on macOS). Scripts see an empty list, a scan that finds nothing, and every open reports the device
// as not connected, as Windows does for one that was unplugged.
//
// The factory keeps its Windows name so the shared callers (src/app/main.cpp) build unchanged.
#include "gs/hardware/Backend.h"

#include <memory>
#include <utility>

namespace gs::hardware {

namespace {

class NoDevicesBackend final : public Backend
{
public:
    [[nodiscard]] std::vector<DeviceInfo> devices() const override { return {}; }

    void watch(std::function<void()>) override {}

    void scan(bool) override {}

    void open(OpenRequest, Opened done) override
    {
        OpenResult result;
        result.problem = "serial, HID and Bluetooth devices are not available on this system yet";
        result.missing = true;
        if (done) {
            done(std::move(result));
        }
    }
};

}

std::unique_ptr<Backend> makeWindowsBackend()
{
    return std::make_unique<NoDevicesBackend>();
}

}
