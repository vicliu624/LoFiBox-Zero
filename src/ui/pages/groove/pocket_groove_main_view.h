// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct PocketGrooveStepCell {
    bool trigger{false};
    bool selected{false};
    bool playhead{false};
    bool locked{false};
};

struct PocketGrooveTrackRow {
    std::string label{};
    std::array<PocketGrooveStepCell, 16> steps{};
};

struct PocketGrooveMainView {
    std::string patternName{"A1"};
    int bpm{90};
    bool playing{false};
    bool chainEnabled{false};
    std::array<bool, 16> filledSlots{};
    std::uint8_t selectedSlot{0};
    std::uint8_t selectedStep{0};
    std::uint8_t selectedTrack{0};
    std::array<PocketGrooveTrackRow, 4> visibleTracks{};
    std::string footer{"S01 EMPTY  STEP01  VEL100"};
    std::uint8_t armedFx{0};
};

void renderPocketGrooveMainView(core::Canvas& canvas, const PocketGrooveMainView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
