// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/export_overlay.h"

#include <algorithm>

#include "ui/pages/groove/groove_ui_helpers.h"
#include "ui/ui_primitives.h"

namespace lofibox::ui::pages::groove {

void renderExportOverlay(core::Canvas& canvas, const ExportOverlayView& view, const UiTheme& theme)
{
    drawGrooveFrame(canvas, theme, view.exporting ? "EXPORTING" : "EXPORT WAV");
    if (view.exporting) {
        ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitUpper(view.fileName, 28), 26, 54, theme.palette.text_primary, 1);
        canvas.fillRect(26, 82, 268, 12, theme.palette.panel2);
        const int filled = std::clamp(view.progressPercent, 0, 100) * 268 / 100;
        canvas.fillRect(26, 82, filled, 12, theme.palette.progress);
        canvas.strokeRect(26, 82, 268, 12, theme.palette.divider, 1);
        ::lofibox::ui::drawText(canvas, std::to_string(std::clamp(view.progressPercent, 0, 100)) + "%", 142, 102, theme.palette.text_secondary, 1);
        return;
    }
    drawGrooveValueRow(canvas, theme, 42, "TARGET", view.target, view.selectedRow == 0);
    drawGrooveValueRow(canvas, theme, 62, "FORMAT", view.format, view.selectedRow == 1);
    drawGrooveValueRow(canvas, theme, 82, "NORM", view.normalize ? "ON" : "OFF", view.selectedRow == 2);
    drawGrooveValueRow(canvas, theme, 102, "TAIL", std::to_string(view.tailSeconds).substr(0, 3) + "S", view.selectedRow == 3);
    drawGrooveFooter(canvas, theme, "OK EXPORT      BACK CANCEL");
}

} // namespace lofibox::ui::pages::groove
