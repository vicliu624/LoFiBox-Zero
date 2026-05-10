// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <vector>

#include "groove/groove_project.h"
#include "groove/groove_step.h"

namespace lofibox::groove {

struct GrooveTriggerEvent {
    std::uint8_t patternIndex{0};
    std::uint8_t trackIndex{0};
    std::uint8_t stepIndex{0};
    std::uint8_t soundSlot{0};
    std::uint8_t velocity{0};
    std::int8_t pitchSemitone{0};
    std::uint8_t sliceIndex{0};
    float gain{1.0f};
    float pan{0.0f};
    std::uint8_t fxType{0};
    float fxAmount{0.0f};
    double startSeconds{0.0};
};

[[nodiscard]] std::vector<GrooveTriggerEvent> collectPatternTriggers(
    const GroovePattern& pattern,
    std::uint8_t pattern_index,
    std::uint16_t bpm,
    std::uint8_t swing,
    double base_seconds = 0.0);

[[nodiscard]] std::vector<GrooveTriggerEvent> collectSongChainTriggers(const GrooveProject& project);
[[nodiscard]] double patternDurationSeconds(const GroovePattern& pattern, std::uint16_t bpm) noexcept;
[[nodiscard]] double songChainDurationSeconds(const GrooveProject& project) noexcept;

} // namespace lofibox::groove
