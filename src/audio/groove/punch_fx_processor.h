// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include "audio/groove/sample_buffer.h"

namespace lofibox::audio::groove {

enum class PunchFxType : std::uint8_t {
    None = 0,
    Filter = 1,
    Stutter = 2,
    Bitcrush = 3,
    TapeStop = 4,
    DelayThrow = 5,
    ReverbFreeze = 6,
    Reverse = 7,
    VinylBrake = 8
};

class PunchFxProcessor {
public:
    void process(SampleBuffer& buffer, PunchFxType type, float amount = 1.0f) const;
};

} // namespace lofibox::audio::groove
