// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lofibox::groove {

struct GrooveSongChainItem {
    std::uint8_t patternIndex{0};
    std::uint8_t repeats{1};

    bool muteMaskEnabled{false};
    std::uint16_t muteMask{0};

    std::string label{};
};

struct GrooveSongChain {
    std::vector<GrooveSongChainItem> items{};
    bool enabled{false};
    std::uint16_t currentItem{0};
};

} // namespace lofibox::groove
