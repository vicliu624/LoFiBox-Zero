// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct MidiOverlayView {
    std::string clock{"INTERNAL"};
    std::string input{"CH 10"};
    std::string output{"OFF"};
    std::string sync{"---"};
    std::string device{"NO DEV"};
    int selectedRow{0};
};

void renderMidiOverlay(core::Canvas& canvas, const MidiOverlayView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
