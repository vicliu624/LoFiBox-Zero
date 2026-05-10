// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/capture_overlay.h"

#include "ui/pages/groove/groove_ui_helpers.h"

namespace lofibox::ui::pages::groove {

void renderCaptureOverlay(core::Canvas& canvas, const CaptureOverlayView& view, const UiTheme& theme)
{
    drawGrooveFrame(canvas, theme, "CAPTURE CURRENT TRACK");
    drawGrooveValueRow(canvas, theme, 38, "POS", view.position, view.selectedRow == 0);
    drawGrooveValueRow(canvas, theme, 56, "LEN", view.length, view.selectedRow == 1);
    drawGrooveValueRow(canvas, theme, 74, "SLOT", view.slot, view.selectedRow == 2);
    drawGrooveValueRow(canvas, theme, 92, "NAME", view.name, view.selectedRow == 3);
    drawGrooveFooter(canvas, theme, "OK REC   LEFT/RIGHT LEN   UP/DOWN SLOT");
}

} // namespace lofibox::ui::pages::groove
