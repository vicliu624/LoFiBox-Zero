// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/project_overlay.h"

#include <array>

#include "ui/pages/groove/groove_ui_helpers.h"
#include "ui/ui_primitives.h"

namespace lofibox::ui::pages::groove {

void renderProjectOverlay(core::Canvas& canvas, const ProjectOverlayView& view, const UiTheme& theme)
{
    drawGrooveFrame(canvas, theme, "PROJECT");
    constexpr std::array<const char*, 4> actions{"SAVE", "LOAD", "NEW", "DELETE"};
    for (int index = 0; index < 4; ++index) {
        const int x = 28 + (index * 70);
        const bool selected = index == view.selectedAction;
        canvas.fillRect(x - 6, 54, 60, 18, selected ? theme.palette.focus_fill : theme.palette.panel2);
        canvas.strokeRect(x - 6, 54, 60, 18, selected ? theme.palette.focus_edge : theme.palette.divider, 1);
        ::lofibox::ui::drawText(canvas, actions[static_cast<std::size_t>(index)], x, 59, theme.palette.text_primary, 1);
    }
    drawGrooveValueRow(canvas, theme, 96, "CURRENT", view.current, false);
    drawGrooveFooter(canvas, theme, "LEFT/RIGHT SELECT   OK RUN   BACK");
}

} // namespace lofibox::ui::pages::groove
