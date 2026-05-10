// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/groove_ui_helpers.h"

#include "ui/ui_primitives.h"

namespace lofibox::ui::pages::groove {

void drawGrooveFrame(core::Canvas& canvas, const UiTheme& theme, std::string_view title)
{
    canvas.clear(theme.palette.background);
    ::lofibox::ui::drawTopBar(canvas, theme, title, true);
    canvas.fillRect(6, 24, 308, 120, theme.palette.panel1);
    canvas.strokeRect(6, 24, 308, 120, theme.palette.divider, 1);
}

void drawGrooveFooter(core::Canvas& canvas, const UiTheme& theme, std::string_view text)
{
    canvas.fillRect(6, 148, 308, 16, theme.palette.panel2);
    canvas.strokeRect(6, 148, 308, 16, theme.palette.divider, 1);
    ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitUpper(text, 38), 12, 152, theme.palette.text_secondary, 1);
}

void drawGrooveValueRow(core::Canvas& canvas, const UiTheme& theme, int y, std::string_view label, std::string_view value, bool selected)
{
    if (selected) {
        ::lofibox::ui::drawGlassListFocus(canvas, theme, 12, y - 2, 296, 13);
    }
    ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitUpper(label, 10), 16, y, theme.palette.text_muted, 1);
    ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitUpper(value, 24), 92, y, selected ? theme.palette.focus_edge : theme.palette.text_primary, 1);
}

} // namespace lofibox::ui::pages::groove
