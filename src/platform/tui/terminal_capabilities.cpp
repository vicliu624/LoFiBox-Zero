// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/tui/terminal_capabilities.h"

#include <cstdlib>
#include <string>

namespace lofibox::platform::tui {

TerminalCapabilities detectTerminalCapabilities()
{
    TerminalCapabilities result{};
    if (const char* no_color = std::getenv("NO_COLOR"); no_color != nullptr) {
        result.color = false;
    }
    if (const char* term = std::getenv("TERM"); term != nullptr && std::string{term} == "dumb") {
        result.color = false;
        result.unicode = false;
        result.preferred_charset = lofibox::tui::TuiCharset::Minimal;
    }
    if (const char* charset = std::getenv("LOFIBOX_TUI_CHARSET"); charset != nullptr && *charset != '\0') {
        result.preferred_charset = lofibox::tui::tuiCharsetFromName(charset, result.preferred_charset);
    } else if (!result.unicode) {
        result.preferred_charset = lofibox::tui::TuiCharset::Ascii;
    }
    return result;
}

} // namespace lofibox::platform::tui

