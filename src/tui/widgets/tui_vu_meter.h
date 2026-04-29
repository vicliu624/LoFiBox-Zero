// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "runtime/runtime_snapshot.h"
#include "tui/tui_state.h"

namespace lofibox::tui::widgets {

[[nodiscard]] std::string renderVuMeter(const runtime::VisualizationRuntimeSnapshot& visualization, int width, TuiCharset charset);

} // namespace lofibox::tui::widgets

