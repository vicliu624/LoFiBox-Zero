// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include "tui/tui_state.h"

namespace lofibox::platform::tui {

[[nodiscard]] std::optional<lofibox::tui::TuiKeyEvent> decodeTerminalInputBytes(std::string_view input);
[[nodiscard]] std::string sanitizeTerminalPaste(std::string_view input);

class TerminalInput {
public:
    TerminalInput() = default;
    ~TerminalInput();

    void enterRawMode();
    void leaveRawMode();
    [[nodiscard]] std::optional<lofibox::tui::TuiKeyEvent> poll(std::chrono::milliseconds timeout);

private:
    bool raw_{false};
#if defined(__unix__) || defined(__APPLE__)
    bool saved_{false};
    struct TermiosHolder;
    TermiosHolder* termios_{nullptr};
#endif
};

} // namespace lofibox::platform::tui
