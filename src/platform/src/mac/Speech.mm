// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Speech synthesis, PCM rendering and voice command recognition on macOS with AVSpeechSynthesizer, AVAudioEngine
// and SFSpeechRecognizer (preferring on-device recognition).
#include "gs/platform/Speech.h"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <Speech/Speech.h>

#include <pthread.h>

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gs::platform {

void speechFinished(void* owner, void* synthesizer, bool finished);

}

@interface GSSpeechDelegate : NSObject <AVSpeechSynthesizerDelegate>
@property(nonatomic, assign) void* owner;
@end

@implementation GSSpeechDelegate
- (void)finish:(AVSpeechSynthesizer*)synthesizer finished:(bool)finished
{
    gs::platform::speechFinished(self.owner, (__bridge void*)synthesizer, finished);
}
- (void)speechSynthesizer:(AVSpeechSynthesizer*)synthesizer didFinishSpeechUtterance:(AVSpeechUtterance*)utterance
{
    (void)utterance;
    [self finish:synthesizer finished:true];
}
- (void)speechSynthesizer:(AVSpeechSynthesizer*)synthesizer didCancelSpeechUtterance:(AVSpeechUtterance*)utterance
{
    (void)utterance;
    [self finish:synthesizer finished:false];
}
@end

namespace gs::platform {

namespace {

constexpr int kSilencePadFrames = SystemSpeech::kRecordedRate / 4;

std::string utf8(NSString* text)
{
    const char* bytes = text != nil ? text.UTF8String : nullptr;
    return bytes != nullptr ? std::string(bytes) : std::string();
}

NSString* nsText(std::string_view text)
{
    NSString* made = [[NSString alloc] initWithBytes:text.data() length:text.size() encoding:NSUTF8StringEncoding];
    return made != nil ? made : @"";
}

std::string bcp47(NSString* identifier)
{
    std::string tag = utf8(identifier);
    std::ranges::replace(tag, '_', '-');
    return tag;
}

std::string loweredAscii(std::string_view text)
{
    std::string out(text);
    std::ranges::transform(out, out.begin(),
                           [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
    return out;
}

std::size_t utf8Step(std::string_view text, std::size_t at, char32_t& codePoint) noexcept
{
    const auto lead = static_cast<unsigned char>(text[at]);
    if (lead < 0x80U) {
        codePoint = lead;
        return 1;
    }
    const std::size_t length = lead < 0xE0U ? 2 : lead < 0xF0U ? 3 : 4;
    if (at + length > text.size()) {
        codePoint = lead;
        return 1;
    }
    codePoint = length == 2 ? (lead & 0x1FU) : length == 3 ? (lead & 0x0FU) : (lead & 0x07U);
    for (std::size_t index = 1; index < length; ++index) {
        codePoint = (codePoint << 6U) | (static_cast<unsigned char>(text[at + index]) & 0x3FU);
    }
    return length;
}

bool ignorableForCommandMatch(char32_t cp) noexcept
{
    switch (cp) {
    case U' ':
    case U'\t':
    case U'\r':
    case U'\n':
    case U'.':
    case U',':
    case U'!':
    case U'?':
    case U';':
    case U':':
    case U'\'':
    case U'"':
    case U'-':
    case U'\u3002': // 。
    case U'\uFF0C': // ，
    case U'\uFF01': // ！
    case U'\uFF1F': // ？
    case U'\u3001': // 、
    case U'\uFF1B': // ；
    case U'\uFF1A': // ：
    case U'\u2018':
    case U'\u2019':
    case U'\u201C':
    case U'\u201D':
        return true;
    default:
        return false;
    }
}

std::string squeezedCommand(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (std::size_t at = 0; at < text.size();) {
        char32_t cp = 0;
        const std::size_t step = utf8Step(text, at, cp);
        if (!ignorableForCommandMatch(cp)) {
            if (cp >= U'A' && cp <= U'Z') {
                out.push_back(static_cast<char>(cp - U'A' + U'a'));
            } else {
                out.append(text.substr(at, step));
            }
        }
        at += step;
    }
    return out;
}

struct NormalizedPhrase {
    std::string original;
    std::string squeezed;
};

std::vector<NormalizedPhrase> normalizePhrases(const std::vector<std::string>& phrases)
{
    std::vector<NormalizedPhrase> out;
    out.reserve(phrases.size());
    for (const std::string& phrase : phrases) {
        std::string squeezed = squeezedCommand(phrase);
        if (!squeezed.empty()) {
            out.push_back({.original = phrase, .squeezed = std::move(squeezed)});
        }
    }
    return out;
}

std::optional<std::string> matchPhrase(const std::vector<NormalizedPhrase>& phrases, std::string_view candidate,
                                       bool finalResult)
{
    const std::string spoken = squeezedCommand(candidate);
    if (spoken.empty() || phrases.empty()) {
        return std::nullopt;
    }

    const NormalizedPhrase* bestExact = nullptr;
    for (const NormalizedPhrase& entry : phrases) {
        if (spoken == entry.squeezed) {
            bestExact = &entry;
            break;
        }
    }
    if (bestExact != nullptr) {
        if (!finalResult) {
            for (const NormalizedPhrase& other : phrases) {
                if (other.squeezed.size() > bestExact->squeezed.size() &&
                    other.squeezed.starts_with(bestExact->squeezed)) {
                    return std::nullopt;
                }
            }
        }
        return bestExact->original;
    }

    const NormalizedPhrase* bestSuffix = nullptr;
    for (const NormalizedPhrase& entry : phrases) {
        if (spoken.ends_with(entry.squeezed)) {
            if (bestSuffix == nullptr || entry.squeezed.size() > bestSuffix->squeezed.size()) {
                bestSuffix = &entry;
            }
        }
    }
    if (bestSuffix != nullptr) {
        if (!finalResult) {
            for (const NormalizedPhrase& other : phrases) {
                if (other.squeezed.size() > bestSuffix->squeezed.size() &&
                    other.squeezed.starts_with(bestSuffix->squeezed)) {
                    return std::nullopt;
                }
            }
        }
        return bestSuffix->original;
    }

    if (finalResult) {
        const NormalizedPhrase* bestContained = nullptr;
        for (const NormalizedPhrase& entry : phrases) {
            if (spoken.find(entry.squeezed) != std::string::npos) {
                if (bestContained == nullptr || entry.squeezed.size() > bestContained->squeezed.size()) {
                    bestContained = &entry;
                }
            }
        }
        if (bestContained != nullptr) {
            return bestContained->original;
        }
    }

    return std::nullopt;
}

double transcriptionConfidence(SFTranscription* transcription)
{
    NSArray<SFTranscriptionSegment*>* segments = transcription.segments;
    if (segments.count == 0) {
        return 0.85;
    }
    double sum = 0.0;
    std::size_t counted = 0;
    for (SFTranscriptionSegment* segment in segments) {
        if (segment.confidence > 0.0F && segment.confidence <= 1.0F) {
            sum += static_cast<double>(segment.confidence);
            ++counted;
        }
    }
    if (counted == 0) {
        return 0.85;
    }
    return std::clamp(sum / static_cast<double>(counted), 0.01, 1.0);
}

AVSpeechSynthesisVoice* findVoice(const std::string& wanted)
{
    if (wanted.empty()) {
        return nil;
    }
    NSString* wantedNs = nsText(wanted);
    // macOS 15 hands back some installed voice for any identifier: only the voice asked for counts.
    AVSpeechSynthesisVoice* direct = [AVSpeechSynthesisVoice voiceWithIdentifier:wantedNs];
    if (direct != nil && [direct.identifier caseInsensitiveCompare:wantedNs] == NSOrderedSame) {
        return direct;
    }
    for (AVSpeechSynthesisVoice* each in [AVSpeechSynthesisVoice speechVoices]) {
        if ([each.identifier caseInsensitiveCompare:wantedNs] == NSOrderedSame ||
            [each.name caseInsensitiveCompare:wantedNs] == NSOrderedSame) {
            return each;
        }
    }
    return nil;
}

AVSpeechUtterance* buildUtterance(const SpeechLine& line, AVSpeechSynthesisVoice* voice, bool silent)
{
    AVSpeechUtterance* utterance = [AVSpeechUtterance speechUtteranceWithString:nsText(line.text)];
    if (voice != nil) {
        utterance.voice = voice;
    }
    utterance.rate = static_cast<float>(std::clamp(line.rate, 0.1, 4.0) * AVSpeechUtteranceDefaultSpeechRate);
    utterance.rate =
        std::clamp(utterance.rate, AVSpeechUtteranceMinimumSpeechRate, AVSpeechUtteranceMaximumSpeechRate);
    utterance.volume = silent ? 0.0F : static_cast<float>(std::clamp(line.volume, 0.0, 1.0));
    return utterance;
}

NSLocale* matchedRecognizerLocale(std::string_view wantedLanguage)
{
    NSSet<NSLocale*>* supported = [SFSpeechRecognizer supportedLocales];
    if (supported.count == 0) {
        return nil;
    }
    std::string wanted(wantedLanguage);
    std::ranges::replace(wanted, '_', '-');
    if (wanted.empty()) {
        wanted = bcp47([[NSLocale currentLocale] localeIdentifier]);
    }
    const std::string lowerWanted = loweredAscii(wanted);
    std::vector<std::pair<std::string, NSLocale*>> entries;
    entries.reserve(supported.count);
    for (NSLocale* locale in supported) {
        entries.emplace_back(bcp47(locale.localeIdentifier), locale);
    }
    std::ranges::sort(entries, [](const auto& left, const auto& right) { return left.first < right.first; });

    for (const auto& [tag, locale] : entries) {
        if (loweredAscii(tag) == lowerWanted) {
            return locale;
        }
    }
    if (lowerWanted == "zh-hans" || lowerWanted.starts_with("zh-hans-")) {
        for (const auto& [tag, locale] : entries) {
            if (loweredAscii(tag) == "zh-cn") {
                return locale;
            }
        }
    }
    if (lowerWanted == "zh-hant" || lowerWanted.starts_with("zh-hant-")) {
        for (const auto& [tag, locale] : entries) {
            if (loweredAscii(tag) == "zh-tw") {
                return locale;
            }
        }
    }
    for (const auto& [tag, locale] : entries) {
        const std::string lowerTag = loweredAscii(tag);
        if (lowerTag.starts_with(lowerWanted + "-") || lowerWanted.starts_with(lowerTag + "-")) {
            return locale;
        }
    }
    return nil;
}

SFSpeechRecognizer* createRecognizer(std::string_view wantedLanguage, std::string& problem)
{
    NSLocale* locale = matchedRecognizerLocale(wantedLanguage);
    if (locale == nil) {
        problem = "macOS has no speech recognizer for " +
                  (wantedLanguage.empty() ? std::string("the current language") : std::string(wantedLanguage));
        return nil;
    }
    SFSpeechRecognizer* recognizer = [[SFSpeechRecognizer alloc] initWithLocale:locale];
    if (recognizer == nil || !recognizer.isAvailable) {
        problem = "the speech recognizer for " + bcp47(locale.localeIdentifier) + " is not available";
        return nil;
    }
    return recognizer;
}

NSArray<NSString*>* contextualStringsOf(const std::vector<std::string>& phrases)
{
    NSMutableArray<NSString*>* array = [NSMutableArray arrayWithCapacity:phrases.size()];
    for (const std::string& phrase : phrases) {
        NSString* item = nsText(phrase);
        if (item.length > 0) {
            [array addObject:item];
        }
    }
    return array;
}

bool bundleHasUsageKey(NSString* key)
{
    id value = [[NSBundle mainBundle] objectForInfoDictionaryKey:key];
    return [value isKindOfClass:[NSString class]] && [(NSString*)value length] > 0;
}

void convertPcmChunk(AVAudioConverter* converter, AVAudioFormat* outFormat, AVAudioPCMBuffer* inBuffer,
                     bool endOfStream, std::vector<std::uint8_t>& outBytes)
{
    if (converter == nil || outFormat == nil) {
        return;
    }
    const double ratio = inBuffer != nil && inBuffer.format.sampleRate > 0.0
                             ? static_cast<double>(SystemSpeech::kRecordedRate) / inBuffer.format.sampleRate
                             : 1.0;
    const AVAudioFrameCount baseFrames = inBuffer != nil ? inBuffer.frameLength : 0;
    const AVAudioFrameCount capacity =
        std::max<AVAudioFrameCount>(256, static_cast<AVAudioFrameCount>(std::ceil(baseFrames * ratio)) + 128);

    __block bool fedInput = inBuffer == nil;
    for (;;) {
        AVAudioPCMBuffer* outBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:outFormat frameCapacity:capacity];
        if (outBuffer == nil) {
            break;
        }
        NSError* error = nil;
        const AVAudioConverterOutputStatus status = [converter
            convertToBuffer:outBuffer
                      error:&error
         withInputFromBlock:^AVAudioBuffer* _Nullable(AVAudioPacketCount, AVAudioConverterInputStatus* inputStatus) {
             if (!fedInput) {
                 fedInput = true;
                 *inputStatus = AVAudioConverterInputStatus_HaveData;
                 return inBuffer;
             }
             *inputStatus = endOfStream ? AVAudioConverterInputStatus_EndOfStream
                                        : AVAudioConverterInputStatus_NoDataNow;
             return nil;
         }];
        if (outBuffer.frameLength > 0 && outBuffer.int16ChannelData != nullptr) {
            const auto* raw = reinterpret_cast<const std::uint8_t*>(outBuffer.int16ChannelData[0]);
            outBytes.insert(outBytes.end(), raw, raw + static_cast<std::size_t>(outBuffer.frameLength) * sizeof(std::int16_t));
        }
        if (status == AVAudioConverterOutputStatus_Error || status == AVAudioConverterOutputStatus_EndOfStream ||
            (!endOfStream && fedInput && outBuffer.frameLength == 0)) {
            break;
        }
        if (outBuffer.frameLength == 0) {
            break;
        }
    }
}

bool isBenignNoSpeechError(NSError* error)
{
    if (error == nil) {
        return false;
    }
    if ([error.domain isEqualToString:@"kAFAssistantErrorDomain"] && (error.code == 1110 || error.code == 203)) {
        return true;
    }
    NSString* desc = error.localizedDescription.lowercaseString;
    return desc != nil && [desc containsString:@"no speech"];
}

struct ListenerEntry {
    std::uint64_t id = 0;
    std::vector<std::string> rawPhrases;
    std::vector<NormalizedPhrase> phrases;
    SystemSpeech::Heard heard;
    SystemSpeech::Stopped stopped;
    SFSpeechRecognizer* recognizer = nil;
    NSOperationQueue* queue = nil;
    SFSpeechAudioBufferRecognitionRequest* request = nil;
    SFSpeechRecognitionTask* task = nil;
    AVAudioEngine* audioEngine = nil;
    bool tapInstalled = false;
    bool cancelled = false;
    bool settled = false;
    NSUInteger consumedChars = 0;

    void teardownAudio()
    {
        if (audioEngine != nil) {
            if (tapInstalled) {
                [audioEngine.inputNode removeTapOnBus:0];
                tapInstalled = false;
            }
            [audioEngine stop];
            audioEngine = nil;
        }
        if (request != nil) {
            [request endAudio];
            request = nil;
        }
        if (task != nil) {
            [task cancel];
            task = nil;
        }
    }
};

struct SpeechState : public std::enable_shared_from_this<SpeechState> {
    struct QueuedSpeech {
        std::uint64_t id = 0;
        SpeechLine line;
        SystemSpeech::Spoken spoken;
    };

    std::mutex mutex;
    std::uint64_t next = 1;
    std::deque<QueuedSpeech> speechQueue;
    std::optional<QueuedSpeech> currentSpeech;
    AVSpeechSynthesizer* currentSpeaker = nil;
    id delegate = nil;
    bool silent = false;

    std::optional<std::vector<std::uint8_t>> heardInput;
    std::map<std::uint64_t, std::shared_ptr<ListenerEntry>> listeners;

    void scheduleNextSpeech()
    {
        std::weak_ptr<SpeechState> weak = shared_from_this();
        dispatch_async(dispatch_get_main_queue(), [weak] {
            if (const std::shared_ptr<SpeechState> self = weak.lock()) {
                self->startNextSpeechOnMain();
            }
        });
    }

    void startNextSpeechOnMain()
    {
        for (;;) {
            QueuedSpeech job;
            AVSpeechSynthesizer* speaker = nil;
            AVSpeechUtterance* utterance = nil;
            bool runSilent = false;
            std::string immediateError;
            {
                const std::scoped_lock lock(mutex);
                if (currentSpeech.has_value() || speechQueue.empty()) {
                    return;
                }
                job = std::move(speechQueue.front());
                speechQueue.pop_front();

                AVSpeechSynthesisVoice* voice = nil;
                if (!job.line.voice.empty()) {
                    voice = findVoice(job.line.voice);
                    if (voice == nil) {
                        immediateError = "the voice " + job.line.voice + " cannot be used";
                    }
                }
                if (immediateError.empty() && !job.line.text.empty()) {
                    utterance = buildUtterance(job.line, voice, silent);
                    speaker = [[AVSpeechSynthesizer alloc] init];
                    runSilent = silent;
                    if (!runSilent) {
                        speaker.delegate = delegate;
                    }
                    currentSpeech = std::move(job);
                    currentSpeaker = speaker;
                }
            }

            if (!immediateError.empty()) {
                if (job.spoken) {
                    job.spoken(false, std::move(immediateError));
                }
                continue;
            }
            if (speaker == nil) {
                if (job.spoken) {
                    job.spoken(true, {});
                }
                continue;
            }

            if (runSilent) {
                const std::uint64_t jobId = currentSpeech->id;
                std::weak_ptr<SpeechState> weak = shared_from_this();
                [speaker writeUtterance:utterance
                       toBufferCallback:^(AVAudioBuffer* _Nonnull buffer) {
                           auto* pcm = (AVAudioPCMBuffer*)buffer;
                           if ([pcm isKindOfClass:[AVAudioPCMBuffer class]] && pcm.frameLength > 0) {
                               return;
                           }
                           if (const std::shared_ptr<SpeechState> self = weak.lock()) {
                               self->completeSpeechId(jobId, true);
                           }
                       }];
            } else {
                [speaker speakUtterance:utterance];
            }
            return;
        }
    }

    void completeSpeechBySynthesizer(void* synthesizer, bool finished)
    {
        SystemSpeech::Spoken callback;
        {
            const std::scoped_lock lock(mutex);
            if (!currentSpeech.has_value() || (__bridge void*)currentSpeaker != synthesizer) {
                return;
            }
            callback = std::move(currentSpeech->spoken);
            currentSpeech.reset();
            currentSpeaker.delegate = nil;
            currentSpeaker = nil;
        }
        if (callback) {
            callback(finished, {});
        }
        scheduleNextSpeech();
    }

    void completeSpeechId(std::uint64_t id, bool finished)
    {
        SystemSpeech::Spoken callback;
        {
            const std::scoped_lock lock(mutex);
            if (!currentSpeech.has_value() || currentSpeech->id != id) {
                return;
            }
            callback = std::move(currentSpeech->spoken);
            currentSpeech.reset();
            currentSpeaker.delegate = nil;
            currentSpeaker = nil;
        }
        if (callback) {
            callback(finished, {});
        }
        scheduleNextSpeech();
    }

    void finishListener(const std::shared_ptr<ListenerEntry>& entry, std::string problem)
    {
        SystemSpeech::Stopped callback;
        {
            const std::scoped_lock lock(mutex);
            if (entry->cancelled || entry->settled) {
                return;
            }
            entry->settled = true;
            listeners.erase(entry->id);
            entry->teardownAudio();
            callback = std::move(entry->stopped);
        }
        if (callback) {
            callback(std::move(problem));
        }
    }

    void startRecordedRecognition(const std::shared_ptr<ListenerEntry>& entry, std::vector<std::uint8_t> pcm)
    {
        if (pcm.size() < sizeof(std::int16_t)) {
            [entry->queue addOperationWithBlock:^{
                finishListener(entry, "the recorded speech cannot be read");
            }];
            return;
        }

        AVAudioFormat* format = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatInt16
                                                                 sampleRate:SystemSpeech::kRecordedRate
                                                                   channels:1
                                                                interleaved:YES];
        const auto speechFrames = static_cast<AVAudioFrameCount>(pcm.size() / sizeof(std::int16_t));
        const AVAudioFrameCount totalFrames = speechFrames + static_cast<AVAudioFrameCount>(kSilencePadFrames);
        AVAudioPCMBuffer* audioBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:totalFrames];
        if (audioBuffer == nil || audioBuffer.int16ChannelData == nullptr) {
            [entry->queue addOperationWithBlock:^{
                finishListener(entry, "the recorded speech cannot be read");
            }];
            return;
        }
        audioBuffer.frameLength = totalFrames;
        std::memcpy(audioBuffer.int16ChannelData[0], pcm.data(), static_cast<std::size_t>(speechFrames) * sizeof(std::int16_t));
        std::memset(audioBuffer.int16ChannelData[0] + speechFrames, 0,
                    static_cast<std::size_t>(kSilencePadFrames) * sizeof(std::int16_t));

        SFSpeechAudioBufferRecognitionRequest* request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
        request.shouldReportPartialResults = NO;
        if (@available(macOS 13.0, *)) {
            request.addsPunctuation = NO;
        }
        if (entry->recognizer.supportsOnDeviceRecognition) {
            request.requiresOnDeviceRecognition = YES;
        }
        request.contextualStrings = contextualStringsOf(entry->rawPhrases);
        [request appendAudioPCMBuffer:audioBuffer];
        [request endAudio];

        std::weak_ptr<SpeechState> weakSelf = shared_from_this();
        std::weak_ptr<ListenerEntry> weakEntry = entry;
        {
            const std::scoped_lock lock(mutex);
            if (entry->cancelled) {
                return;
            }
            entry->request = request;
            entry->task = [entry->recognizer
                recognitionTaskWithRequest:request
                             resultHandler:^(SFSpeechRecognitionResult* _Nullable result, NSError* _Nullable error) {
                                 const std::shared_ptr<SpeechState> self = weakSelf.lock();
                                 const std::shared_ptr<ListenerEntry> active = weakEntry.lock();
                                 if (!self || !active) {
                                     return;
                                 }
                                 if (result != nil && result.isFinal) {
                                     SystemSpeech::Heard heardCallback;
                                     std::optional<SpeechHeard> matched;
                                     {
                                         const std::scoped_lock innerLock(self->mutex);
                                         if (!active->cancelled && !active->settled) {
                                             const std::string text = utf8(result.bestTranscription.formattedString);
                                             if (const auto phrase = matchPhrase(active->phrases, text, true)) {
                                                 matched = SpeechHeard{
                                                     .phrase = *phrase,
                                                     .text = text,
                                                     .confidence = transcriptionConfidence(result.bestTranscription),
                                                 };
                                                 heardCallback = active->heard;
                                             }
                                         }
                                     }
                                     if (matched && heardCallback) {
                                         heardCallback(std::move(*matched));
                                     }
                                     self->finishListener(active, {});
                                     return;
                                 }
                                 if (error != nil) {
                                     if (isBenignNoSpeechError(error)) {
                                         self->finishListener(active, {});
                                         return;
                                     }
                                     const SFSpeechRecognizerAuthorizationStatus auth =
                                         [SFSpeechRecognizer authorizationStatus];
                                     if (auth == SFSpeechRecognizerAuthorizationStatusDenied ||
                                         auth == SFSpeechRecognizerAuthorizationStatusRestricted) {
                                         self->finishListener(
                                             active,
                                             "speech recognition is not allowed in System Settings > Privacy & "
                                             "Security > Speech Recognition");
                                         return;
                                     }
                                     self->finishListener(active, utf8(error.localizedDescription));
                                 }
                             }];
        }
    }

    void armLiveRecognitionTaskLocked(const std::shared_ptr<ListenerEntry>& entry)
    {
        if (entry->cancelled || entry->settled || entry->recognizer == nil) {
            return;
        }
        if (entry->task != nil) {
            [entry->task cancel];
            entry->task = nil;
        }
        if (entry->request != nil) {
            [entry->request endAudio];
            entry->request = nil;
        }
        entry->consumedChars = 0;

        SFSpeechAudioBufferRecognitionRequest* request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
        request.shouldReportPartialResults = YES;
        if (@available(macOS 13.0, *)) {
            request.addsPunctuation = NO;
        }
        if (entry->recognizer.supportsOnDeviceRecognition) {
            request.requiresOnDeviceRecognition = YES;
        }
        request.contextualStrings = contextualStringsOf(entry->rawPhrases);
        entry->request = request;

        std::weak_ptr<SpeechState> weakSelf = shared_from_this();
        std::weak_ptr<ListenerEntry> weakEntry = entry;
        entry->task = [entry->recognizer
            recognitionTaskWithRequest:request
                         resultHandler:^(SFSpeechRecognitionResult* _Nullable result, NSError* _Nullable error) {
                             const std::shared_ptr<SpeechState> self = weakSelf.lock();
                             const std::shared_ptr<ListenerEntry> active = weakEntry.lock();
                             if (!self || !active) {
                                 return;
                             }
                             if (result != nil) {
                                 NSString* full = result.bestTranscription.formattedString;
                                 const bool isFinal = result.isFinal;
                                 SystemSpeech::Heard heardCallback;
                                 std::optional<SpeechHeard> matched;
                                 {
                                     const std::scoped_lock lock(self->mutex);
                                     if (active->cancelled || active->settled || active->request != request) {
                                         return;
                                     }
                                     if (full.length < active->consumedChars) {
                                         active->consumedChars = 0;
                                     }
                                     NSString* tailNs = [full substringFromIndex:active->consumedChars];
                                     const std::string tail = utf8(tailNs);
                                     if (const auto phrase = matchPhrase(active->phrases, tail, isFinal)) {
                                         active->consumedChars = full.length;
                                         matched = SpeechHeard{
                                             .phrase = *phrase,
                                             .text = tail,
                                             .confidence = transcriptionConfidence(result.bestTranscription),
                                         };
                                         heardCallback = active->heard;
                                     }
                                 }
                                 if (matched && heardCallback) {
                                     heardCallback(std::move(*matched));
                                 }
                                 if (isFinal) {
                                     const std::scoped_lock lock(self->mutex);
                                     if (!active->cancelled && !active->settled && active->request == request) {
                                         self->armLiveRecognitionTaskLocked(active);
                                     }
                                     return;
                                 }
                             }
                             if (error != nil) {
                                 if (isBenignNoSpeechError(error)) {
                                     const std::scoped_lock lock(self->mutex);
                                     if (!active->cancelled && !active->settled && active->request == request) {
                                         self->armLiveRecognitionTaskLocked(active);
                                     }
                                     return;
                                 }
                                 const SFSpeechRecognizerAuthorizationStatus auth =
                                     [SFSpeechRecognizer authorizationStatus];
                                 if (auth == SFSpeechRecognizerAuthorizationStatusDenied ||
                                     auth == SFSpeechRecognizerAuthorizationStatusRestricted) {
                                     self->finishListener(
                                         active,
                                         "speech recognition is not allowed in System Settings > Privacy & "
                                         "Security > Speech Recognition");
                                     return;
                                 }
                                 self->finishListener(active, utf8(error.localizedDescription));
                             }
                         }];
    }

    void startMicrophoneEngine(const std::shared_ptr<ListenerEntry>& entry)
    {
        if ([AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio] == nil) {
            finishListener(entry, "there is no microphone to listen with");
            return;
        }

        AVAudioEngine* engine = [[AVAudioEngine alloc] init];
        AVAudioInputNode* inputNode = engine.inputNode;
        if (inputNode == nil) {
            finishListener(entry, "there is no microphone to listen with");
            return;
        }
        AVAudioFormat* inputFormat = [inputNode outputFormatForBus:0];
        if (inputFormat == nil || inputFormat.channelCount == 0 || inputFormat.sampleRate <= 0.0) {
            finishListener(entry, "there is no microphone to listen with");
            return;
        }

        std::weak_ptr<SpeechState> weakSelf = shared_from_this();
        std::weak_ptr<ListenerEntry> weakEntry = entry;
        NSError* tapError = nil;
        bool tapOk = true;
        {
            const std::scoped_lock lock(mutex);
            if (entry->cancelled || entry->settled) {
                return;
            }
            entry->audioEngine = engine;
            armLiveRecognitionTaskLocked(entry);
            AVAudioNodeTapBlock tapBlock = ^(AVAudioPCMBuffer* _Nonnull buffer, AVAudioTime* _Nonnull) {
                const std::shared_ptr<SpeechState> self = weakSelf.lock();
                const std::shared_ptr<ListenerEntry> active = weakEntry.lock();
                if (!self || !active) {
                    return;
                }
                SFSpeechAudioBufferRecognitionRequest* currentReq = nil;
                {
                    const std::scoped_lock tapLock(self->mutex);
                    if (!active->cancelled && !active->settled) {
                        currentReq = active->request;
                    }
                }
                if (currentReq != nil) {
                    [currentReq appendAudioPCMBuffer:buffer];
                }
            };
            // The variant that reports an error is in the macOS 27 SDK; an older SDK (as on the CI's Xcode) only has
            // the one that throws.
#if defined(__MAC_27_0) && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_27_0
            if (@available(macOS 27.0, *)) {
                tapOk = [inputNode installTapOnBus:0 bufferSize:1024 format:inputFormat error:&tapError block:tapBlock];
            } else
#endif
            {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                @try {
                    [inputNode installTapOnBus:0 bufferSize:1024 format:inputFormat block:tapBlock];
                } @catch (NSException*) {
                    tapOk = false;
                }
#pragma clang diagnostic pop
            }
            entry->tapInstalled = tapOk;
        }
        if (!tapOk) {
            finishListener(entry, tapError != nil ? utf8(tapError.localizedDescription)
                                                  : std::string("the microphone could not start"));
            return;
        }

        [engine prepare];
        NSError* startError = nil;
        if (![engine startAndReturnError:&startError]) {
            finishListener(entry, startError != nil ? utf8(startError.localizedDescription)
                                                    : std::string("the microphone could not start"));
        }
    }

    void authorizeAndStartMicrophone(const std::shared_ptr<ListenerEntry>& entry)
    {
        const AVAuthorizationStatus micStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
        if (micStatus == AVAuthorizationStatusDenied || micStatus == AVAuthorizationStatusRestricted) {
            finishListener(entry,
                           "microphone access is not allowed in System Settings > Privacy & Security > Microphone");
            return;
        }
        if (micStatus == AVAuthorizationStatusNotDetermined) {
            if (!bundleHasUsageKey(@"NSMicrophoneUsageDescription")) {
                finishListener(entry, "microphone access requires running from anmacgamer.app");
                return;
            }
            std::weak_ptr<SpeechState> weakSelf = shared_from_this();
            std::weak_ptr<ListenerEntry> weakEntry = entry;
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
                                     completionHandler:^(BOOL granted) {
                                         const std::shared_ptr<SpeechState> self = weakSelf.lock();
                                         const std::shared_ptr<ListenerEntry> active = weakEntry.lock();
                                         if (!self || !active) {
                                             return;
                                         }
                                         if (!granted) {
                                             self->finishListener(
                                                 active,
                                                 "microphone access is not allowed in System Settings > Privacy & "
                                                 "Security > Microphone");
                                             return;
                                         }
                                         [active->queue addOperationWithBlock:^{
                                             self->authorizeSpeechAndStartMicrophone(active);
                                         }];
                                     }];
            return;
        }
        authorizeSpeechAndStartMicrophone(entry);
    }

    void authorizeSpeechAndStartMicrophone(const std::shared_ptr<ListenerEntry>& entry)
    {
        const SFSpeechRecognizerAuthorizationStatus srStatus = [SFSpeechRecognizer authorizationStatus];
        if (srStatus == SFSpeechRecognizerAuthorizationStatusDenied ||
            srStatus == SFSpeechRecognizerAuthorizationStatusRestricted) {
            finishListener(
                entry,
                "speech recognition is not allowed in System Settings > Privacy & Security > Speech Recognition");
            return;
        }
        if (srStatus == SFSpeechRecognizerAuthorizationStatusNotDetermined) {
            if (!bundleHasUsageKey(@"NSSpeechRecognitionUsageDescription")) {
                finishListener(entry, "speech recognition requires running from anmacgamer.app");
                return;
            }
            std::weak_ptr<SpeechState> weakSelf = shared_from_this();
            std::weak_ptr<ListenerEntry> weakEntry = entry;
            [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {
                const std::shared_ptr<SpeechState> self = weakSelf.lock();
                const std::shared_ptr<ListenerEntry> active = weakEntry.lock();
                if (!self || !active) {
                    return;
                }
                if (status != SFSpeechRecognizerAuthorizationStatusAuthorized) {
                    self->finishListener(active,
                                         "speech recognition is not allowed in System Settings > Privacy & Security > "
                                         "Speech Recognition");
                    return;
                }
                [active->queue addOperationWithBlock:^{
                    self->startMicrophoneEngine(active);
                }];
            }];
            return;
        }
        startMicrophoneEngine(entry);
    }
};

} // namespace

struct SystemSpeech::Impl {
    std::shared_ptr<SpeechState> state = std::make_shared<SpeechState>();
};

void speechFinished(void* owner, void* synthesizer, bool finished)
{
    if (owner == nullptr) {
        return;
    }
    const std::shared_ptr<SpeechState> state = static_cast<SystemSpeech*>(owner)->m_impl->state;
    state->completeSpeechBySynthesizer(synthesizer, finished);
}

SystemSpeech::SystemSpeech(SpeechOutput output) : m_impl(std::make_unique<Impl>())
{
    m_impl->state->silent = output == SpeechOutput::Nowhere;
    GSSpeechDelegate* delegate = [[GSSpeechDelegate alloc] init];
    delegate.owner = this;
    m_impl->state->delegate = delegate;
}

SystemSpeech::~SystemSpeech()
{
    const std::shared_ptr<SpeechState>& shared = m_impl->state;
    AVSpeechSynthesizer* activeSpeaker = nil;
    std::deque<SpeechState::QueuedSpeech> leftSpeech;
    std::optional<SpeechState::QueuedSpeech> interruptedSpeech;
    std::vector<std::shared_ptr<ListenerEntry>> activeListeners;
    {
        const std::scoped_lock lock(shared->mutex);
        if (GSSpeechDelegate* delegate = (GSSpeechDelegate*)shared->delegate) {
            delegate.owner = nullptr;
        }
        activeSpeaker = shared->currentSpeaker;
        if (activeSpeaker != nil) {
            activeSpeaker.delegate = nil;
        }
        shared->currentSpeaker = nil;
        interruptedSpeech = std::move(shared->currentSpeech);
        shared->currentSpeech.reset();
        leftSpeech.swap(shared->speechQueue);

        for (auto& [id, listener] : shared->listeners) {
            listener->cancelled = true;
            listener->teardownAudio();
            activeListeners.push_back(std::move(listener));
        }
        shared->listeners.clear();
    }
    if (activeSpeaker != nil) {
        [activeSpeaker stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
    }
    if (interruptedSpeech && interruptedSpeech->spoken) {
        interruptedSpeech->spoken(false, {});
    }
    for (SpeechState::QueuedSpeech& queued : leftSpeech) {
        if (queued.spoken) {
            queued.spoken(false, {});
        }
    }
}

std::vector<SpeechVoice> SystemSpeech::voices() const
{
    std::vector<SpeechVoice> found;
    @autoreleasepool {
        for (AVSpeechSynthesisVoice* voice in [AVSpeechSynthesisVoice speechVoices]) {
            SpeechVoice item{
                .id = utf8(voice.identifier),
                .name = utf8(voice.name),
                .language = bcp47(voice.language),
            };
            if (!item.id.empty() && !item.name.empty()) {
                found.push_back(std::move(item));
            }
        }
    }
    return found;
}

std::vector<std::string> SystemSpeech::languages() const
{
    std::vector<std::string> found;
    @autoreleasepool {
        for (NSLocale* locale in [SFSpeechRecognizer supportedLocales]) {
            std::string tag = bcp47(locale.localeIdentifier);
            if (!tag.empty() && tag.find('-') != std::string::npos &&
                std::ranges::find(found, tag) == found.end()) {
                found.push_back(std::move(tag));
            }
        }
    }
    std::ranges::sort(found);
    return found;
}

std::uint64_t SystemSpeech::say(SpeechLine line, Spoken spoken)
{
    const std::shared_ptr<SpeechState>& impl = m_impl->state;
    std::uint64_t id = 0;
    {
        const std::scoped_lock lock(impl->mutex);
        id = impl->next++;
        impl->speechQueue.push_back({.id = id, .line = std::move(line), .spoken = std::move(spoken)});
    }
    impl->scheduleNextSpeech();
    return id;
}

void SystemSpeech::stop(std::uint64_t id)
{
    if (id == 0) {
        return;
    }
    const std::shared_ptr<SpeechState>& impl = m_impl->state;
    Spoken dropped;
    AVSpeechSynthesizer* speakerToStop = nil;
    {
        const std::scoped_lock lock(impl->mutex);
        if (impl->currentSpeech.has_value() && impl->currentSpeech->id == id) {
            dropped = std::move(impl->currentSpeech->spoken);
            impl->currentSpeech.reset();
            speakerToStop = impl->currentSpeaker;
            if (speakerToStop != nil) {
                speakerToStop.delegate = nil;
            }
            impl->currentSpeaker = nil;
        } else if (const auto queued = std::ranges::find(impl->speechQueue, id, &SpeechState::QueuedSpeech::id);
                   queued != impl->speechQueue.end()) {
            dropped = std::move(queued->spoken);
            impl->speechQueue.erase(queued);
        }
    }
    if (speakerToStop != nil) {
        [speakerToStop stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
        impl->scheduleNextSpeech();
    }
    if (dropped) {
        dropped(false, {});
    }
}

std::uint64_t SystemSpeech::listen(SpeechCommands commands, Heard heard, Stopped stopped)
{
    const std::shared_ptr<SpeechState>& impl = m_impl->state;

    @autoreleasepool {
        auto entry = std::make_shared<ListenerEntry>();
        entry->rawPhrases = std::move(commands.phrases);
        entry->phrases = normalizePhrases(entry->rawPhrases);
        entry->heard = std::move(heard);
        entry->stopped = std::move(stopped);

        NSOperationQueue* queue = [[NSOperationQueue alloc] init];
        queue.maxConcurrentOperationCount = 1;
        entry->queue = queue;

        std::optional<std::vector<std::uint8_t>> recordedInput;
        {
            const std::scoped_lock lock(impl->mutex);
            entry->id = impl->next++;
            recordedInput = std::exchange(impl->heardInput, std::nullopt);
            impl->listeners.emplace(entry->id, entry);
        }

        if (entry->phrases.empty()) {
            [queue addOperationWithBlock:^{
                impl->finishListener(entry, "no commands to listen for");
            }];
            return entry->id;
        }

        std::string recognizerProblem;
        SFSpeechRecognizer* recognizer = createRecognizer(commands.language, recognizerProblem);
        if (recognizer == nil) {
            [queue addOperationWithBlock:^{
                impl->finishListener(entry, recognizerProblem);
            }];
            return entry->id;
        }
        recognizer.queue = queue;
        entry->recognizer = recognizer;

        if (recordedInput.has_value()) {
            impl->startRecordedRecognition(entry, std::move(*recordedInput));
        } else {
            [queue addOperationWithBlock:^{
                impl->authorizeAndStartMicrophone(entry);
            }];
        }
        return entry->id;
    }
}

void SystemSpeech::unlisten(std::uint64_t id)
{
    if (id == 0) {
        return;
    }
    const std::shared_ptr<SpeechState>& impl = m_impl->state;
    std::shared_ptr<ListenerEntry> removed;
    {
        const std::scoped_lock lock(impl->mutex);
        const auto found = impl->listeners.find(id);
        if (found != impl->listeners.end()) {
            removed = std::move(found->second);
            removed->cancelled = true;
            removed->teardownAudio();
            impl->listeners.erase(found);
        }
    }
}

std::optional<std::vector<std::uint8_t>> SystemSpeech::record(const SpeechLine& line)
{
    if (line.text.empty()) {
        return std::nullopt;
    }

    @autoreleasepool {
        AVSpeechSynthesisVoice* voice = nil;
        if (!line.voice.empty()) {
            voice = findVoice(line.voice);
            if (voice == nil) {
                return std::nullopt;
            }
        }

        AVAudioFormat* outFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatInt16
                                                                    sampleRate:kRecordedRate
                                                                      channels:1
                                                                   interleaved:YES];
        if (outFormat == nil) {
            return std::nullopt;
        }

        struct RecordState {
            std::mutex mutex;
            std::condition_variable cv;
            AVSpeechSynthesizer* synthesizer = nil;
            AVAudioConverter* converter = nil;
            std::vector<std::uint8_t> pcm;
            bool finished = false;
            bool failed = false;
        };
        auto state = std::make_shared<RecordState>();

        SpeechLine lineCopy = line;
        auto startOnMain = [state, lineCopy = std::move(lineCopy), voice, outFormat] {
            @autoreleasepool {
                AVSpeechUtterance* utterance = buildUtterance(lineCopy, voice, false);
                AVSpeechSynthesizer* synthesizer = [[AVSpeechSynthesizer alloc] init];
                {
                    const std::scoped_lock lock(state->mutex);
                    state->synthesizer = synthesizer;
                }
                [synthesizer writeUtterance:utterance
                           toBufferCallback:^(AVAudioBuffer* _Nonnull buffer) {
                               auto* inBuffer = (AVAudioPCMBuffer*)buffer;
                               const std::scoped_lock lock(state->mutex);
                               if (state->finished) {
                                   return;
                               }
                               if (![inBuffer isKindOfClass:[AVAudioPCMBuffer class]] || inBuffer.frameLength == 0) {
                                   if (state->converter != nil) {
                                       convertPcmChunk(state->converter, outFormat, nil, true, state->pcm);
                                   }
                                   state->finished = true;
                                   state->synthesizer = nil;
                                   state->cv.notify_all();
                                   return;
                               }
                               if (state->converter == nil) {
                                   state->converter = [[AVAudioConverter alloc] initFromFormat:inBuffer.format
                                                                                      toFormat:outFormat];
                                   if (state->converter == nil) {
                                       state->failed = true;
                                       state->finished = true;
                                       state->synthesizer = nil;
                                       state->cv.notify_all();
                                       return;
                                   }
                               }
                               convertPcmChunk(state->converter, outFormat, inBuffer, false, state->pcm);
                           }];
            }
        };

        if (pthread_main_np() != 0) {
            startOnMain();
            NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:15.0];
            for (;;) {
                {
                    const std::scoped_lock lock(state->mutex);
                    if (state->finished) {
                        break;
                    }
                }
                if ([deadline timeIntervalSinceNow] <= 0.0) {
                    break;
                }
                [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                         beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
            }
        } else {
            dispatch_async(dispatch_get_main_queue(), startOnMain);
            std::unique_lock lock(state->mutex);
            state->cv.wait_for(lock, std::chrono::seconds(15), [&state] { return state->finished; });
        }

        const std::scoped_lock lock(state->mutex);
        if (!state->finished || state->failed) {
            if (state->synthesizer != nil) {
                [state->synthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
                state->synthesizer = nil;
            }
            return std::nullopt;
        }
        return std::move(state->pcm);
    }
}

void SystemSpeech::hearFrom(std::vector<std::uint8_t> pcm)
{
    const std::scoped_lock lock(m_impl->state->mutex);
    m_impl->state->heardInput = std::move(pcm);
}

} // namespace gs::platform
