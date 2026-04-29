// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "tui/tui_state.h"

namespace lofibox::tui {

struct TuiLayout {
    TuiLayoutKind kind{TuiLayoutKind::Normal};
    TerminalSize size{};
    bool show_header{true};
    bool show_spectrum{true};
    bool show_lyrics{true};
    bool show_queue{true};
    bool show_footer{true};
};

[[nodiscard]] TuiLayout classifyTuiLayout(TerminalSize size, const TuiOptions& options = {});

} // namespace lofibox::tui

