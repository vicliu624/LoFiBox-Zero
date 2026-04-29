// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/spectrum_tui_page.h"

#include "tui/widgets/tui_spectrum.h"
#include "tui/widgets/tui_text.h"
#include "tui/widgets/tui_vu_meter.h"

namespace lofibox::tui::pages {

std::vector<std::string> spectrumPageLines(const TuiModel& model, const TuiLayout& layout)
{
    std::vector<std::string> lines{};
    lines.push_back(widgets::fitText("Spectrum · " + model.snapshot.visualization.mode, layout.size.width));
    for (const auto& line : widgets::renderSpectrumBlock(model.snapshot.visualization, layout.size.width, layout.size.height - 4, model.options.charset)) {
        lines.push_back(widgets::fitText(line, layout.size.width));
    }
    lines.push_back(widgets::fitText("VU " + widgets::renderVuMeter(model.snapshot.visualization, layout.size.width - 3, model.options.charset), layout.size.width));
    return lines;
}

} // namespace lofibox::tui::pages

