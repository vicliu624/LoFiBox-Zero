// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <sstream>

#include "platform/tui/terminal_capabilities.h"
#include "platform/tui/terminal_input.h"
#include "platform/tui/terminal_presenter.h"
#include "tui/tui_state.h"

int main()
{
    const auto capabilities = lofibox::platform::tui::detectTerminalCapabilities();
    if (capabilities.preferred_charset != lofibox::tui::TuiCharset::Unicode
        && capabilities.preferred_charset != lofibox::tui::TuiCharset::Ascii
        && capabilities.preferred_charset != lofibox::tui::TuiCharset::Minimal) {
        std::cerr << "Expected terminal charset capability to be classified.\n";
        return 1;
    }

    std::ostringstream output{};
    lofibox::platform::tui::TerminalPresenter presenter{output};
    lofibox::platform::tui::TerminalDiffRenderer diff{presenter};
    lofibox::tui::TuiFrame first{10, 3};
    first.writeLine(0, "first");
    diff.draw(first);
    lofibox::tui::TuiFrame second{10, 3};
    second.writeLine(0, "second");
    diff.draw(second);
    diff.forceFullRepaint();
    diff.draw(second);
    if (output.str().find("first") == std::string::npos || output.str().find("second") == std::string::npos) {
        std::cerr << "Expected diff renderer to emit full and changed frames.\n";
        return 1;
    }

    const auto utf8 = lofibox::platform::tui::decodeTerminalInputBytes("中");
    if (!utf8 || utf8->key != lofibox::tui::TuiKey::Character || utf8->codepoint != U'中' || utf8->text != "中") {
        std::cerr << "Expected terminal input decoder to preserve committed UTF-8 text.\n";
        return 1;
    }
    const auto paste = lofibox::platform::tui::decodeTerminalInputBytes("\x1b[200~alpha\n\x1b[31mbeta\x1b[201~");
    if (!paste || paste->key != lofibox::tui::TuiKey::Paste || paste->text != "alpha beta") {
        std::cerr << "Expected terminal input decoder to guard bracketed paste and strip ANSI controls.\n";
        return 1;
    }
    const auto backtab = lofibox::platform::tui::decodeTerminalInputBytes("\x1b[Z");
    if (!backtab || backtab->key != lofibox::tui::TuiKey::BackTab) {
        std::cerr << "Expected terminal input decoder to recognize Shift+Tab.\n";
        return 1;
    }
    return 0;
}
