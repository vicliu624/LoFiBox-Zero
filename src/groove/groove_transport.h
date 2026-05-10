// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <vector>

namespace lofibox::groove {

struct GrooveStepTiming {
    std::uint8_t stepIndex{0};
    double startSeconds{0.0};
    double durationSeconds{0.0};
};

[[nodiscard]] double secondsPerBeat(std::uint16_t bpm) noexcept;
[[nodiscard]] double secondsPerStep(std::uint16_t bpm) noexcept;
[[nodiscard]] double swingOffsetSeconds(std::uint16_t bpm, std::uint8_t swing, std::uint8_t step_index) noexcept;
[[nodiscard]] std::vector<GrooveStepTiming> buildStepTiming(std::uint16_t bpm, std::uint8_t swing, std::uint8_t length);

} // namespace lofibox::groove
