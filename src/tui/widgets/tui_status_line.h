// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "tui/tui_model.h"

namespace lofibox::tui::widgets {

[[nodiscard]] std::string renderStatusLine(const TuiModel& model, int width);
[[nodiscard]] std::string renderFooterLine(const TuiModel& model, int width);

} // namespace lofibox::tui::widgets

