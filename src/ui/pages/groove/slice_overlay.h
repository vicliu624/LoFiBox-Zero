// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct SliceOverlayView {
    std::string title{"SLICE SLOT 04"};
    std::uint8_t sliceCount{8};
    std::uint8_t selectedSlice{2};
    std::string range{"00.420-00.610"};
    std::string assign{"STEP 05"};
};

void renderSliceOverlay(core::Canvas& canvas, const SliceOverlayView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
