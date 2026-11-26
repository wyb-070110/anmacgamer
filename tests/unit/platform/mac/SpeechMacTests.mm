// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Speech synthesis, 16 kHz mono PCM rendering and voice command phrase matching on macOS.
#include "gs/platform/Speech.h"

#include <doctest/doctest.h>

#import <Foundation/Foundation.h>
#import <Speech/Speech.h>

#include <pthread.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using gs::platform::SpeechHeard;
using gs::platform::SpeechOutput;
using gs::platform::SpeechVoice;
using gs::platform::SystemSpeech;

struct Waiter {
    std::mutex mutex;
    std::condition_variable changed;

    template <typename Ready>
    bool until(Ready ready)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        if (pthread_main_np() != 0) {
            for (;;) {
                {
                    const std::scoped_lock lock(mutex);
                    if (ready()) {
                        return true;
                    }
                }
                if (std::chrono::steady_clock::now() >= deadline) {
                    return false;
                }
                @autoreleasepool {
                    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                             beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.005]];
                }
            }
        }
        std::unique_lock lock(mutex);
        return changed.wait_until(lock, deadline, ready);
    }

    template <typename Change>
    void note(Change change)
    {
        {
            const std::scoped_lock lock(mutex);
            change();
        }
        changed.notify_all();
    }
};

std::optional<SpeechVoice> voiceForLanguage(const SystemSpeech& speech, std::string_view language)
{
    const std::vector<SpeechVoice> voices = speech.voices();
    const std::vector<std::string> languages = speech.languages();
    if (std::ranges::find(languages, language) == languages.end()) {
        return std::nullopt;
    }
    const auto found = std::ranges::find(voices, language, &SpeechVoice::language);
    if (found == voices.end()) {
        return std::nullopt;
    }
    return *found;
}

bool hasNonZeroSample(const std::vector<std::uint8_t>& pcm)
{
    for (std::size_t at = 0; at + 1 < pcm.size(); at += 2) {
        std::int16_t sample = 0;
        std::memcpy(&sample, pcm.data() + at, sizeof(sample));
        if (sample != 0) {
            return true;
        }
    }
    return false;
}

bool speechRecognitionDeniedBySystem()
{
    const SFSpeechRecognizerAuthorizationStatus status = [SFSpeechRecognizer authorizationStatus];
    return status == SFSpeechRecognizerAuthorizationStatusDenied ||
           status == SFSpeechRecognizerAuthorizationStatusRestricted;
}

} // namespace

TEST_CASE("macOS speech lists voices and BCP-47 recognition languages, and renders 16 kHz mono PCM")
{
    const SystemSpeech speech(SpeechOutput::Nowhere);
    const std::vector<SpeechVoice> voices = speech.voices();
    const std::vector<std::string> languages = speech.languages();
    MESSAGE(voices.size(), " voices, recognizers for ", languages.size(), " languages");
    CHECK_FALSE(voices.empty());
    CHECK_FALSE(languages.empty());

    for (const SpeechVoice& voice : voices) {
        CHECK_FALSE(voice.id.empty());
        CHECK_FALSE(voice.name.empty());
        CHECK(voice.language.find('-') != std::string::npos);
    }
    for (const std::string& language : languages) {
        CHECK(language.find('-') != std::string::npos);
    }

    CHECK_FALSE(SystemSpeech::record({.text = ""}).has_value());
    CHECK_FALSE(SystemSpeech::record({.text = "hello", .voice = "com.example.voice.nonexistent"}).has_value());

    if (!voices.empty()) {
        const auto voice = voiceForLanguage(speech, "en-US").value_or(voices.front());
        const std::optional<std::vector<std::uint8_t>> pcm =
            SystemSpeech::record({.text = "Ready", .voice = voice.id, .rate = 1.0, .volume = 1.0});
        REQUIRE(pcm.has_value());
        CHECK(pcm->size() % sizeof(std::int16_t) == 0);
        CHECK(pcm->size() > SystemSpeech::kRecordedRate / 4);
        CHECK(hasNonZeroSample(*pcm));
    }
}

TEST_CASE("macOS speech matches Chinese and English commands from recorded 16 kHz PCM and ignores unrelated words")
{
    if (speechRecognitionDeniedBySystem()) {
        MESSAGE("Speech Recognition is denied in System Settings; skipping recognizer check");
        return;
    }
    if (std::getenv("GITHUB_ACTIONS") != nullptr) {
        MESSAGE("GitHub's virtual Macs never answer a recognition request; skipping recognizer check");
        return;
    }

    SystemSpeech speech(SpeechOutput::Nowhere);

    if (const std::optional<SpeechVoice> zhVoice = voiceForLanguage(speech, "zh-CN")) {
        const auto pcm = SystemSpeech::record({.text = "打开地图", .voice = zhVoice->id, .rate = 1.0, .volume = 1.0});
        REQUIRE(pcm.has_value());
        CHECK(pcm->size() > SystemSpeech::kRecordedRate / 2);

        Waiter waiter;
        std::vector<SpeechHeard> heard;
        std::optional<std::string> stopped;
        speech.hearFrom(*pcm);
        speech.listen({.phrases = {"关闭背包", "打开地图", "向前走"}, .language = "zh-CN"},
                      [&](SpeechHeard got) { waiter.note([&] { heard.push_back(std::move(got)); }); },
                      [&](std::string problem) { waiter.note([&] { stopped = std::move(problem); }); });
        REQUIRE(waiter.until([&] { return stopped.has_value(); }));
        if (!stopped->empty()) {
            MESSAGE("on-device zh-CN model unavailable in this environment: ", *stopped);
        } else {
            REQUIRE(heard.size() == 1);
            CHECK(heard[0].phrase == "打开地图");
            CHECK(heard[0].confidence > 0.0);
            CHECK(heard[0].confidence <= 1.0);
        }

        const auto unrelated =
            SystemSpeech::record({.text = "今天天气很好", .voice = zhVoice->id, .rate = 1.0, .volume = 1.0});
        REQUIRE(unrelated.has_value());
        heard.clear();
        stopped.reset();
        speech.hearFrom(*unrelated);
        speech.listen({.phrases = {"关闭背包", "打开地图"}, .language = "zh-CN"},
                      [&](SpeechHeard got) { waiter.note([&] { heard.push_back(std::move(got)); }); },
                      [&](std::string problem) { waiter.note([&] { stopped = std::move(problem); }); });
        REQUIRE(waiter.until([&] { return stopped.has_value(); }));
        if (stopped->empty()) {
            CHECK(heard.empty());
        }
    }

    if (const std::optional<SpeechVoice> enVoice = voiceForLanguage(speech, "en-US")) {
        const auto pcm = SystemSpeech::record({.text = "open map", .voice = enVoice->id, .rate = 1.0, .volume = 1.0});
        REQUIRE(pcm.has_value());

        Waiter waiter;
        std::vector<SpeechHeard> heard;
        std::optional<std::string> stopped;
        speech.hearFrom(*pcm);
        speech.listen({.phrases = {"Close Bag", "Open Map", "Jump"}, .language = "en-US"},
                      [&](SpeechHeard got) { waiter.note([&] { heard.push_back(std::move(got)); }); },
                      [&](std::string problem) { waiter.note([&] { stopped = std::move(problem); }); });
        REQUIRE(waiter.until([&] { return stopped.has_value(); }));
        if (!stopped->empty()) {
            MESSAGE("on-device en-US model unavailable in this environment: ", *stopped);
        } else {
            REQUIRE(heard.size() == 1);
            CHECK(heard[0].phrase == "Open Map");
            CHECK(heard[0].confidence > 0.0);
            CHECK(heard[0].confidence <= 1.0);
        }
    }
}

TEST_CASE("macOS speech reads lines in turn, drops a stopped one, and reports bad voices and languages")
{
    SystemSpeech speech(SpeechOutput::Nowhere);
    if (speech.voices().empty()) {
        MESSAGE("no voice on this computer");
        return;
    }

    Waiter waiter;
    std::vector<std::pair<int, bool>> spoken;
    std::string problem;
    const auto noting = [&](int line) {
        return [&waiter, &spoken, &problem, line](bool finished, std::string why) {
            waiter.note([&] {
                spoken.emplace_back(line, finished);
                if (!why.empty()) {
                    problem = std::move(why);
                }
            });
        };
    };

    speech.say({.text = "First line."}, noting(1));
    const std::uint64_t second = speech.say({.text = "Second line."}, noting(2));
    speech.say({.text = "Third line."}, noting(3));
    speech.stop(second);
    REQUIRE(waiter.until([&] { return spoken.size() == 3; }));
    CHECK(std::ranges::find(spoken, std::pair(2, false)) != spoken.end());
    const auto first = std::ranges::find(spoken, std::pair(1, true));
    const auto third = std::ranges::find(spoken, std::pair(3, true));
    CHECK(first != spoken.end());
    CHECK(third != spoken.end());
    CHECK(first < third);
    CHECK(problem.empty());

    speech.say({.text = "Missing voice", .voice = "com.example.voice.nobody"}, noting(4));
    REQUIRE(waiter.until([&] { return spoken.size() == 4; }));
    CHECK(spoken.back() == std::pair(4, false));
    CHECK(problem.find("cannot be used") != std::string::npos);

    std::optional<std::string> stopped;
    speech.listen({.phrases = {"hello"}, .language = "xx-XX"}, [](SpeechHeard) {},
                  [&](std::string why) { waiter.note([&] { stopped = std::move(why); }); });
    REQUIRE(waiter.until([&] { return stopped.has_value(); }));
    CHECK(stopped->find("no speech recognizer") != std::string::npos);
}
