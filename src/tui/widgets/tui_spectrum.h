// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "runtime/runtime_snapshot.h"
#include "tui/tui_state.h"

namespace lofibox::tui::widgets {

[[nodiscard]] std::string renderSpectrumBars(const runtime::VisualizationRuntimeSnapshot& visualization, int bands, TuiCharset charset);
[[nodiscard]] std::vector<std::string> renderSpectrumBlock(const runtime::VisualizationRuntimeSnapshot& visualization, int width, int height, TuiCharset charset);

} // namespace lofibox::tui::widgets

