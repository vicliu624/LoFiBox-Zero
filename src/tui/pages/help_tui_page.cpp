// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/help_tui_page.h"

#include "tui/widgets/tui_help_overlay.h"

namespace lofibox::tui::pages {

std::vector<std::string> helpPageLines(const TuiModel&, const TuiLayout& layout)
{
    return widgets::helpOverlayLines(layout.size.width);
}

} // namespace lofibox::tui::pages

