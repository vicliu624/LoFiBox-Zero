// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>

#include "audio/groove/sample_buffer.h"

namespace lofibox::audio::groove {

class GrooveMixer {
public:
    void mixInto(SampleBuffer& target, const SampleBuffer& source, std::size_t start_frame, float gain = 1.0f, float pan = 0.0f) const noexcept;
    void clamp(SampleBuffer& target) const noexcept;
};

} // namespace lofibox::audio::groove
