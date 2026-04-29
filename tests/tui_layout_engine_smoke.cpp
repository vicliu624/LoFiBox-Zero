// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "tui/tui_layout_engine.h"

int main()
{
    if (lofibox::tui::classifyTuiLayout({24, 6}).kind != lofibox::tui::TuiLayoutKind::TooSmall) {
        std::cerr << "Expected too-small layout below 32x8.\n";
        return 1;
    }
    if (lofibox::tui::classifyTuiLayout({32, 8}).kind != lofibox::tui::TuiLayoutKind::Tiny) {
        std::cerr << "Expected 32x8 tiny layout.\n";
        return 1;
    }
    if (lofibox::tui::classifyTuiLayout({80, 24}).kind != lofibox::tui::TuiLayoutKind::Normal) {
        std::cerr << "Expected 80x24 normal layout.\n";
        return 1;
    }
    if (lofibox::tui::classifyTuiLayout({120, 40}).kind != lofibox::tui::TuiLayoutKind::Wide) {
        std::cerr << "Expected 120x40 wide layout.\n";
        return 1;
    }
    return 0;
}

