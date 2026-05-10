// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct CaptureOverlayView {
    std::string position{"00:00.00"};
    std::string length{"1 BAR"};
    std::string slot{"04 EMPTY"};
    std::string name{"CHOP_04"};
    int selectedRow{1};
};

void renderCaptureOverlay(core::Canvas& canvas, const CaptureOverlayView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
