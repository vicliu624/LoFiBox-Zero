// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "tui/tui_state.h"

namespace lofibox::tui::widgets {

struct BoxGlyphs {
    std::string top_left{"┌"};
    std::string top_right{"┐"};
    std::string bottom_left{"└"};
    std::string bottom_right{"┘"};
    std::string horizontal{"─"};
    std::string vertical{"│"};
};

[[nodiscard]] BoxGlyphs boxGlyphs(TuiCharset charset);
void drawBox(TuiFrame& frame, int row, int col, int width, int height, TuiCharset charset, std::string_view title = {});

} // namespace lofibox::tui::widgets

