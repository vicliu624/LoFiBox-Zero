// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct ProjectOverlayView {
    std::string current{"LATEBEAT"};
    int selectedAction{0};
};

void renderProjectOverlay(core::Canvas& canvas, const ProjectOverlayView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
