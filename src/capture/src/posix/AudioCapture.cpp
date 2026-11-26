// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Sound from a capture card off Windows is not ported yet.
#include "gs/capture/AudioCapture.h"

namespace gs::capture {

struct AudioCapture::Wake {};

AudioCapture::AudioCapture(std::wstring endpoint, Samples samples)
    : m_endpoint(std::move(endpoint)), m_samples(std::move(samples)), m_wake(std::make_unique<Wake>())
{
    fail("Sound from a capture card is not available on this system yet");
}

AudioCapture::~AudioCapture() = default;

std::string AudioCapture::problem() const
{
    const std::scoped_lock lock(m_mutex);
    return m_problem;
}

void AudioCapture::run(const std::stop_token&) {}

void AudioCapture::fail(std::string reason)
{
    const std::scoped_lock lock(m_mutex);
    m_problem = std::move(reason);
}

}
