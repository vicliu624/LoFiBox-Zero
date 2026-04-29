// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/eq_tui_page.h"

#include "tui/widgets/tui_eq_view.h"

namespace lofibox::tui::pages {

std::vector<std::string> eqPageLines(const TuiModel& model, const TuiLayout& layout)
{
    return widgets::renderEqView(model.snapshot.eq, layout.size.width, model.options.charset);
}

} // namespace lofibox::tui::pages

