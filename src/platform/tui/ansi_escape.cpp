// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/tui/ansi_escape.h"

namespace lofibox::platform::tui {

std::string clearScreen() { return "\x1b[2J\x1b[H"; }
std::string enterAlternateScreen() { return "\x1b[?1049h"; }
std::string leaveAlternateScreen() { return "\x1b[?1049l"; }
std::string hideCursor() { return "\x1b[?25l"; }
std::string showCursor() { return "\x1b[?25h"; }
std::string resetStyle() { return "\x1b[0m"; }

std::string moveCursor(int row, int col)
{
    return "\x1b[" + std::to_string(row + 1) + ";" + std::to_string(col + 1) + "H";
}

} // namespace lofibox::platform::tui

