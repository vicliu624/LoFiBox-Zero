// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/slice_overlay.h"

#include <algorithm>

#include "ui/pages/groove/groove_ui_helpers.h"
#include "ui/ui_primitives.h"

namespace lofibox::ui::pages::groove {

void renderSliceOverlay(core::Canvas& canvas, const SliceOverlayView& view, const UiTheme& theme)
{
    drawGrooveFrame(canvas, theme, view.title + " AUTO " + std::to_string(static_cast<int>(view.sliceCount)));
    const int count = std::clamp<int>(view.sliceCount, 1, 16);
    for (int index = 0; index < count; ++index) {
        const int x = 28 + (index * 17);
        const bool selected = index == static_cast<int>(view.selectedSlice);
        canvas.fillRect(x, 48, 14, 16, selected ? theme.palette.focus_fill : theme.palette.panel2);
        canvas.strokeRect(x, 48, 14, 16, selected ? theme.palette.focus_edge : theme.palette.divider, 1);
        ::lofibox::ui::drawText(canvas, std::to_string((index + 1) % 10), x + 4, 52, theme.palette.text_primary, 1);
    }
    drawGrooveValueRow(canvas, theme, 82, "SLICE", std::to_string(static_cast<int>(view.selectedSlice + 1)) + "  " + view.range, true);
    drawGrooveValueRow(canvas, theme, 104, "ASSIGN", view.assign, false);
    drawGrooveFooter(canvas, theme, "OK ASSIGN   PLUS AUTO   BACK");
}

} // namespace lofibox::ui::pages::groove
