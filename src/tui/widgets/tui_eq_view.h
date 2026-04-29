// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "runtime/runtime_snapshot.h"
#include "tui/tui_state.h"

namespace lofibox::tui::widgets {

[[nodiscard]] std::vector<std::string> renderEqView(const runtime::EqRuntimeSnapshot& eq, int width, TuiCharset charset);

} // namespace lofibox::tui::widgets

