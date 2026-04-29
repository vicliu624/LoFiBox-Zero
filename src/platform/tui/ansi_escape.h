// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace lofibox::platform::tui {

[[nodiscard]] std::string clearScreen();
[[nodiscard]] std::string enterAlternateScreen();
[[nodiscard]] std::string leaveAlternateScreen();
[[nodiscard]] std::string hideCursor();
[[nodiscard]] std::string showCursor();
[[nodiscard]] std::string moveCursor(int row, int col);
[[nodiscard]] std::string resetStyle();

} // namespace lofibox::platform::tui

