// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "tui/widgets/tui_progress_bar.h"

int main()
{
    const auto zero = lofibox::tui::widgets::renderProgressLine(12.0, 0, false, 32, lofibox::tui::TuiCharset::Ascii);
    if (zero.empty() || zero.find("0:12") == std::string::npos) {
        std::cerr << "Expected zero-duration progress to render without division by zero.\n";
        return 1;
    }

    const auto live = lofibox::tui::widgets::renderProgressLine(0.0, 0, true, 32, lofibox::tui::TuiCharset::Ascii);
    if (live.find("LIVE") == std::string::npos || live.find("0:00") != std::string::npos) {
        std::cerr << "Expected live progress to use live semantics.\n";
        return 1;
    }
    return 0;
}

