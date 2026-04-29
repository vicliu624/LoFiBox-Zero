// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "tui/tui_layout_engine.h"
#include "tui/tui_model.h"

namespace lofibox::tui {

class TuiRenderer {
public:
    [[nodiscard]] TuiFrame render(const TuiModel& model, TerminalSize size) const;
};

} // namespace lofibox::tui

