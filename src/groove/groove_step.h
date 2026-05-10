// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

namespace lofibox::groove {

struct GrooveStep {
    bool trigger{false};

    std::uint8_t velocity{100};
    std::int8_t pitchSemitone{0};
    std::int8_t microTiming{0};

    std::uint8_t sliceIndex{0};

    bool hasGainLock{false};
    float gain{1.0f};

    bool hasPanLock{false};
    float pan{0.0f};

    bool hasFilterLock{false};
    float filterCutoff{1.0f};

    bool hasFxLock{false};
    std::uint8_t fxType{0};
    float fxAmount{0.0f};
};

} // namespace lofibox::groove
