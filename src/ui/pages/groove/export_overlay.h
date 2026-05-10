// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct ExportOverlayView {
    std::string target{"SONG CHAIN"};
    std::string format{"48K / 16"};
    bool normalize{true};
    double tailSeconds{2.0};
    bool exporting{false};
    int progressPercent{0};
    std::string fileName{"LATEBEAT_001.WAV"};
    int selectedRow{0};
};

void renderExportOverlay(core::Canvas& canvas, const ExportOverlayView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
