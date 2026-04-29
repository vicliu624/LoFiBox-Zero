// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "runtime/runtime_snapshot.h"

namespace lofibox::tui::widgets {

[[nodiscard]] std::vector<std::string> renderQueueView(const runtime::QueueRuntimeSnapshot& queue, int max_lines, int width);

} // namespace lofibox::tui::widgets

