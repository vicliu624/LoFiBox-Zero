// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/widgets/tui_vu_meter.h"

#include <algorithm>

#include "tui/widgets/tui_progress_bar.h"

namespace lofibox::tui::widgets {

std::string renderVuMeter(const runtime::VisualizationRuntimeSnapshot& visualization, int width, TuiCharset charset)
{
    const auto level = std::max(visualization.rms_left, visualization.rms_right);
    return renderProgressBar(level * 100.0, 100, false, std::max(1, width), charset);
}

} // namespace lofibox::tui::widgets

