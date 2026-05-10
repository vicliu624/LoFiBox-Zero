// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct ChainOverlayItemView {
    std::string pattern{"A1"};
    std::uint8_t repeats{4};
};

struct ChainOverlayView {
    std::vector<ChainOverlayItemView> items{};
    std::uint8_t selectedItem{0};
};

void renderChainOverlay(core::Canvas& canvas, const ChainOverlayView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
