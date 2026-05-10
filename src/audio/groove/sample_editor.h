// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "audio/groove/sample_buffer.h"
#include "groove/groove_sound_slot.h"

namespace lofibox::audio::groove {

struct SampleEditResult {
    bool ok{false};
    SampleBuffer buffer{};
    std::string errorMessage{};
};

class SampleEditor {
public:
    [[nodiscard]] SampleEditResult trim(const SampleBuffer& input, double start_seconds, double end_seconds) const;
    [[nodiscard]] SampleEditResult normalize(const SampleBuffer& input, float target_peak = 0.95f) const;
    [[nodiscard]] SampleEditResult fadeIn(const SampleBuffer& input, double fade_ms) const;
    [[nodiscard]] SampleEditResult fadeOut(const SampleBuffer& input, double fade_ms) const;
    [[nodiscard]] SampleEditResult reverse(const SampleBuffer& input) const;
    [[nodiscard]] std::vector<lofibox::groove::SampleSlice> autoSlice(const SampleBuffer& input, std::uint8_t max_slices) const;
};

} // namespace lofibox::audio::groove
