// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "tui/tui_state.h"

namespace lofibox::platform::tui {

struct TerminalCapabilities {
    bool color{true};
    bool unicode{true};
    lofibox::tui::TuiCharset preferred_charset{lofibox::tui::TuiCharset::Unicode};
};

[[nodiscard]] TerminalCapabilities detectTerminalCapabilities();

} // namespace lofibox::platform::tui

