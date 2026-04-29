// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/command_palette_tui_page.h"

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {

std::vector<std::string> commandPalettePageLines(const TuiModel& model, const TuiLayout& layout)
{
    return {
        widgets::fitText("Command Palette", layout.size.width),
        widgets::fitText(": " + model.command_buffer, layout.size.width),
        widgets::fitText("Allowed: play, pause, next, remote reconnect, eq reset", layout.size.width),
        widgets::fitText("Shell commands are not accepted.", layout.size.width),
    };
}

} // namespace lofibox::tui::pages

