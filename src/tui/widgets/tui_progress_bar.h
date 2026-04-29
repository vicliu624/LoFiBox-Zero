// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "tui/tui_state.h"

namespace lofibox::tui::widgets {

[[nodiscard]] std::string formatDuration(double seconds);
[[nodiscard]] std::string renderProgressBar(double elapsed_seconds, int duration_seconds, bool live, int width, TuiCharset charset);
[[nodiscard]] std::string renderProgressLine(double elapsed_seconds, int duration_seconds, bool live, int width, TuiCharset charset);

} // namespace lofibox::tui::widgets

