// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Sound off Windows through SDL3: the default playback device pulls from the regulator in SDL's callback,
// with the gain ramped as on Windows. SDL's default device follows the system's choice by itself. Without
// SDL3 (the Linux test bed) the output stays silent.
#include "AudioOutput.h"

#include "gs/base/Log.h"
#include "gs/platform/Thread.h"

#if defined(GS_HAVE_SDL3)
#include <SDL3/SDL.h>
#endif

#include <condition_variable>
#include <mutex>
#include <span>
#include <string_view>
#include <vector>

namespace gs::media {

namespace {

constexpr std::string_view kLog = "audio";
[[maybe_unused]] constexpr int kRequestedFrames = 256;

[[maybe_unused]] void applyGain(std::span<float> samples, int channels, float& current, float target)
{
    if (current == target) {
        if (target != 1.0f) {
            for (float& sample : samples) {
                sample *= target;
            }
        }
        return;
    }
    const std::size_t frames = samples.size() / static_cast<std::size_t>(channels);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float gain = current + (target - current) * static_cast<float>(frame + 1) / static_cast<float>(frames);
        for (int channel = 0; channel < channels; ++channel) {
            samples[frame * static_cast<std::size_t>(channels) + static_cast<std::size_t>(channel)] *= gain;
        }
    }
    current = target;
}

}

struct AudioOutput::Wake {
    std::mutex mutex;
    std::condition_variable_any stopped;
    float gain = 1.0f;
    std::vector<float> samples;
};

AudioOutput::AudioOutput(AudioRegulator& source, int channels, int sampleRate, const std::atomic<float>& gain)
    : m_source(source),
      m_channels(channels),
      m_sampleRate(sampleRate),
      m_gain(gain),
      m_wake(std::make_unique<Wake>()),
      m_thread([this](const std::stop_token& stop) { run(stop); })
{
}

AudioOutput::~AudioOutput() = default;

#if defined(GS_HAVE_SDL3)
namespace {

struct Feed {
    AudioRegulator* source = nullptr;
    const std::atomic<float>* gain = nullptr;
    float* current = nullptr;
    std::vector<float>* samples = nullptr;
    int channels = 0;
};

void SDLCALL pull(void* opaque, SDL_AudioStream* stream, int additional, int)
{
    auto& feed = *static_cast<Feed*>(opaque);
    if (additional <= 0) {
        return;
    }
    const std::size_t count = static_cast<std::size_t>(additional) / sizeof(float);
    feed.samples->resize(count - count % static_cast<std::size_t>(feed.channels));
    const std::span<float> samples(*feed.samples);
    feed.source->pull(samples);
    applyGain(samples, feed.channels, *feed.current, feed.gain->load(std::memory_order_relaxed));
    SDL_PutAudioStreamData(stream, samples.data(), static_cast<int>(samples.size_bytes()));
}

}

void AudioOutput::run(const std::stop_token& stop)
{
    platform::setCurrentThreadName("gs-audio-out");
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        log::error(kLog, "no sound: SDL audio is unavailable ({})", SDL_GetError());
        return;
    }
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, std::to_string(kRequestedFrames).c_str());
    m_wake->gain = m_gain.load(std::memory_order_relaxed);
    Feed feed{&m_source, &m_gain, &m_wake->gain, &m_wake->samples, m_channels};
    const SDL_AudioSpec spec{SDL_AUDIO_F32, m_channels, m_sampleRate};
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &pull, &feed);
    if (stream == nullptr) {
        log::warning(kLog, "no sound: the output device cannot be opened ({})", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }
    SDL_AudioSpec device{};
    int frames = 0;
    if (SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &device, &frames) && frames > 0) {
        m_period.store(static_cast<std::uint32_t>(frames), std::memory_order_relaxed);
        m_lowLatency.store(frames <= kRequestedFrames, std::memory_order_relaxed);
        log::info(kLog, "playing on the default device: period of {} frames ({:.1f} ms) at {} Hz", frames,
                  frames * 1000.0 / device.freq, device.freq);
    }
    SDL_ResumeAudioStreamDevice(stream);
    {
        std::unique_lock lock(m_wake->mutex);
        m_wake->stopped.wait(lock, stop, [] { return false; });
    }
    SDL_DestroyAudioStream(stream);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
#else
void AudioOutput::run(const std::stop_token& stop)
{
    platform::setCurrentThreadName("gs-audio-out");
    log::warning(kLog, "no sound: this build has no audio output for {} channels at {} Hz (gain {:.2f}, {} frames waiting)",
                 m_channels, m_sampleRate, m_gain.load(std::memory_order_relaxed), m_source.buffered());
    std::unique_lock lock(m_wake->mutex);
    m_wake->stopped.wait(lock, stop, [] { return false; });
}
#endif

}
