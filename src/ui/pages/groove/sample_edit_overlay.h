// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct SampleEditOverlayView {
    std::string slotTitle{"EDIT SLOT 04"};
    std::string name{"CHOP_04"};
    double startSeconds{0.120};
    double endSeconds{1.880};
    int gain{86};
    int pitch{0};
    int selectedRow{0};
};

void renderSampleEditOverlay(core::Canvas& canvas, const SampleEditOverlayView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
