// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/dsp_tui_page.h"

#include "tui/widgets/tui_eq_view.h"
#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {

std::vector<std::string> dspPageLines(const TuiModel& model, const TuiLayout& layout)
{
    auto lines = widgets::renderEqView(model.snapshot.eq, layout.size.width, model.options.charset);
    lines.insert(lines.begin(), widgets::fitText("DSP Chain", layout.size.width));
    lines.push_back(widgets::fitText("ReplayGain, limiter, loudness: pending runtime projection", layout.size.width));
    return lines;
}

} // namespace lofibox::tui::pages

