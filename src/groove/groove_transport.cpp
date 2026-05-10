// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_transport.h"

#include <algorithm>

namespace lofibox::groove {

double secondsPerBeat(std::uint16_t bpm) noexcept
{
    const auto safe_bpm = std::max<std::uint16_t>(1, bpm);
    return 60.0 / static_cast<double>(safe_bpm);
}

double secondsPerStep(std::uint16_t bpm) noexcept
{
    return secondsPerBeat(bpm) / 4.0;
}

double swingOffsetSeconds(std::uint16_t bpm, std::uint8_t swing, std::uint8_t step_index) noexcept
{
    if ((step_index % 2U) == 0U) {
        return 0.0;
    }
    const double amount = static_cast<double>(std::clamp<std::uint8_t>(swing, 0, 75)) / 100.0;
    return secondsPerStep(bpm) * amount * 0.5;
}

std::vector<GrooveStepTiming> buildStepTiming(std::uint16_t bpm, std::uint8_t swing, std::uint8_t length)
{
    const auto safe_length = static_cast<std::uint8_t>(std::clamp<int>(length, 1, 16));
    std::vector<GrooveStepTiming> timings{};
    timings.reserve(safe_length);

    const double step = secondsPerStep(bpm);
    for (std::uint8_t index = 0; index < safe_length; ++index) {
        GrooveStepTiming timing{};
        timing.stepIndex = index;
        timing.startSeconds = (static_cast<double>(index) * step) + swingOffsetSeconds(bpm, swing, index);
        timing.durationSeconds = step;
        timings.push_back(timing);
    }
    return timings;
}

} // namespace lofibox::groove
