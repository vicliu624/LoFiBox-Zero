// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/sample_edit_overlay.h"

#include <iomanip>
#include <sstream>

#include "ui/pages/groove/groove_ui_helpers.h"
#include "ui/ui_primitives.h"

namespace lofibox::ui::pages::groove {
namespace {

std::string secondsText(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value;
    return out.str();
}

} // namespace

void renderSampleEditOverlay(core::Canvas& canvas, const SampleEditOverlayView& view, const UiTheme& theme)
{
    drawGrooveFrame(canvas, theme, view.slotTitle + " " + view.name);
    canvas.fillRect(32, 38, 256, 16, theme.palette.panel2);
    canvas.strokeRect(32, 38, 256, 16, theme.palette.divider, 1);
    canvas.fillRect(88, 43, 112, 6, theme.palette.focus_fill);
    canvas.fillRect(32, 45, 56, 2, theme.palette.text_muted);
    canvas.fillRect(200, 45, 88, 2, theme.palette.text_muted);
    drawGrooveValueRow(canvas, theme, 64, "START", secondsText(view.startSeconds), view.selectedRow == 0);
    drawGrooveValueRow(canvas, theme, 82, "END", secondsText(view.endSeconds), view.selectedRow == 1);
    drawGrooveValueRow(canvas, theme, 100, "GAIN", std::to_string(view.gain), view.selectedRow == 2);
    drawGrooveValueRow(canvas, theme, 118, "PITCH", (view.pitch >= 0 ? "+" : "") + std::to_string(view.pitch), view.selectedRow == 3);
    drawGrooveFooter(canvas, theme, "OK PLAY   FN TOOL   BACK DONE");
}

} // namespace lofibox::ui::pages::groove
