// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_sequencer.h"

#include <algorithm>

#include "groove/groove_transport.h"

namespace lofibox::groove {

std::vector<GrooveTriggerEvent> collectPatternTriggers(
    const GroovePattern& pattern,
    std::uint8_t pattern_index,
    std::uint16_t bpm,
    std::uint8_t swing,
    double base_seconds)
{
    std::vector<GrooveTriggerEvent> events{};
    const auto timings = buildStepTiming(bpm, swing, pattern.length);
    for (std::size_t track_index = 0; track_index < pattern.tracks.size(); ++track_index) {
        const auto& track = pattern.tracks[track_index];
        if (track.mute) {
            continue;
        }
        for (const auto& timing : timings) {
            const auto& step = track.steps[timing.stepIndex];
            if (!step.trigger) {
                continue;
            }
            GrooveTriggerEvent event{};
            event.patternIndex = pattern_index;
            event.trackIndex = static_cast<std::uint8_t>(track_index);
            event.stepIndex = timing.stepIndex;
            event.soundSlot = track.soundSlot;
            event.velocity = step.velocity;
            event.pitchSemitone = step.pitchSemitone;
            event.sliceIndex = step.sliceIndex;
            event.gain = step.hasGainLock ? step.gain : track.gain;
            event.pan = step.hasPanLock ? step.pan : track.pan;
            event.fxType = step.hasFxLock ? step.fxType : 0;
            event.fxAmount = step.hasFxLock ? step.fxAmount : 0.0f;
            event.startSeconds = base_seconds + timing.startSeconds + (static_cast<double>(step.microTiming) * secondsPerStep(bpm) / 96.0);
            events.push_back(event);
        }
    }
    std::sort(events.begin(), events.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.startSeconds < rhs.startSeconds;
    });
    return events;
}

std::vector<GrooveTriggerEvent> collectSongChainTriggers(const GrooveProject& project)
{
    std::vector<GrooveTriggerEvent> events{};
    if (!project.songChain.enabled || project.songChain.items.empty()) {
        const auto pattern_index = std::min<std::uint8_t>(project.activePattern, static_cast<std::uint8_t>(project.patterns.size() - 1U));
        return collectPatternTriggers(project.patterns[pattern_index], pattern_index, project.bpm, project.swing);
    }

    double cursor = 0.0;
    for (const auto& item : project.songChain.items) {
        const auto pattern_index = std::min<std::uint8_t>(item.patternIndex, static_cast<std::uint8_t>(project.patterns.size() - 1U));
        const auto& pattern = project.patterns[pattern_index];
        const int repeats = std::max<int>(1, item.repeats);
        for (int repeat = 0; repeat < repeats; ++repeat) {
            auto pattern_events = collectPatternTriggers(pattern, pattern_index, project.bpm, project.swing, cursor);
            events.insert(events.end(), pattern_events.begin(), pattern_events.end());
            cursor += patternDurationSeconds(pattern, project.bpm);
        }
    }
    std::sort(events.begin(), events.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.startSeconds < rhs.startSeconds;
    });
    return events;
}

double patternDurationSeconds(const GroovePattern& pattern, std::uint16_t bpm) noexcept
{
    return static_cast<double>(std::clamp<int>(pattern.length, 1, 16)) * secondsPerStep(bpm);
}

double songChainDurationSeconds(const GrooveProject& project) noexcept
{
    if (!project.songChain.enabled || project.songChain.items.empty()) {
        return patternDurationSeconds(project.patterns[project.activePattern], project.bpm);
    }
    double total = 0.0;
    for (const auto& item : project.songChain.items) {
        const auto pattern_index = std::min<std::uint8_t>(item.patternIndex, static_cast<std::uint8_t>(project.patterns.size() - 1U));
        total += patternDurationSeconds(project.patterns[pattern_index], project.bpm) * static_cast<double>(std::max<int>(1, item.repeats));
    }
    return total;
}

} // namespace lofibox::groove
