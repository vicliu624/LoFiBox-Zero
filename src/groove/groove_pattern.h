// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "groove/groove_track.h"

namespace lofibox::groove {

struct GroovePattern {
    std::string name{"A1"};
    std::uint8_t length{16};

    std::array<GrooveTrack, kGrooveTrackCount> tracks{};
};

} // namespace lofibox::groove
