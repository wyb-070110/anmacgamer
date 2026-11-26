// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Opening a capture card off Windows is not ported yet; the source says so when started.
#include "gs/capture/MediaFoundationSource.h"

namespace gs::capture {

struct MediaFoundationSource::Shared {};

MediaFoundationSource::MediaFoundationSource(std::wstring link, std::wstring container, ID3D11Device* device)
    : m_link(std::move(link)), m_container(std::move(container)), m_device(device), m_shared(std::make_shared<Shared>())
{
}

MediaFoundationSource::~MediaFoundationSource() = default;

bool MediaFoundationSource::start(SampleSink&, const std::string&)
{
    return false;
}

void MediaFoundationSource::stop()
{
    static_cast<void>(m_device);
}

std::string MediaFoundationSource::problem() const
{
    return "Capture cards are not available on this system yet";
}

void MediaFoundationSource::run(const std::stop_token&, std::string) {}

}
