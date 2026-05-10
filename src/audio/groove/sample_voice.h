// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>

#include "audio/groove/sample_buffer.h"

namespace lofibox::audio::groove {

struct SampleVoice {
    const SampleBuffer* source{};
    std::size_t positionFrame{0};
    float gain{1.0f};
    bool active{false};

    void trigger(const SampleBuffer& buffer, float voice_gain = 1.0f) noexcept;
    [[nodiscard]] float nextSample(int channel) noexcept;
};

} // namespace lofibox::audio::groove
