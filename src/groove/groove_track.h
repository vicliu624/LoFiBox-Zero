// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "groove/groove_step.h"

namespace lofibox::groove {

inline constexpr std::size_t kGrooveStepCount = 16;
inline constexpr std::size_t kGrooveTrackCount = 16;
inline constexpr std::size_t kGrooveSoundSlotCount = 16;
inline constexpr std::size_t kGroovePatternCount = 16;

struct GrooveTrack {
    std::uint8_t soundSlot{0};

    std::array<GrooveStep, kGrooveStepCount> steps{};

    float gain{1.0f};
    float pan{0.0f};

    bool mute{false};
    bool solo{false};
};

} // namespace lofibox::groove
