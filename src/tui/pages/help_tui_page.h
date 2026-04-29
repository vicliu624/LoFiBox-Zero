// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "tui/tui_layout_engine.h"
#include "tui/tui_model.h"

namespace lofibox::tui::pages {

[[nodiscard]] std::vector<std::string> helpPageLines(const TuiModel& model, const TuiLayout& layout);

} // namespace lofibox::tui::pages

