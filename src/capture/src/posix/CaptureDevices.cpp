// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Capture cards off Windows are not listed yet (AVFoundation would list them on macOS).
#include "gs/capture/CaptureDevices.h"

#include <cstdint>
#include <cwctype>
#include <format>

namespace gs::capture {

namespace {

constexpr std::uint32_t kFnvBasis = 2166136261U;
constexpr std::uint32_t kFnvPrime = 16777619U;

}

bool builtInCamera(const CaptureDeviceInfo& device) noexcept
{
    return device.panelLocated && !device.removable;
}

std::string shortHash(std::wstring_view text)
{
    std::uint32_t hash = kFnvBasis;
    for (const wchar_t character : text) {
        const auto lower = static_cast<std::uint32_t>(std::towlower(static_cast<wint_t>(character)));
        hash ^= lower & 0xFFU;
        hash *= kFnvPrime;
        hash ^= (lower >> 8U) & 0xFFU;
        hash *= kFnvPrime;
    }
    return std::format("{:08x}", hash);
}

std::vector<CaptureDeviceInfo> listCaptureDevices()
{
    return {};
}

struct DeviceWatcher::Impl {};

DeviceWatcher::DeviceWatcher(Changed) : m_impl(std::make_unique<Impl>()) {}

DeviceWatcher::~DeviceWatcher() = default;

bool DeviceWatcher::started() const noexcept
{
    return false;
}

}
