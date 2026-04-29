// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iosfwd>
#include <optional>

#include "tui/tui_state.h"

namespace lofibox::platform::tui {

class TerminalPresenter {
public:
    explicit TerminalPresenter(std::ostream& output);

    [[nodiscard]] lofibox::tui::TerminalSize size() const;
    void enterAlternateScreen();
    void leaveAlternateScreen();
    void hideCursor();
    void showCursor();
    void clear();
    void moveCursor(int row, int col);
    void write(std::string_view text);
    void flush();
    [[nodiscard]] bool colorSupported() const;
    [[nodiscard]] bool unicodeSupported() const;

private:
    std::ostream& output_;
};

class TerminalDiffRenderer {
public:
    explicit TerminalDiffRenderer(TerminalPresenter& presenter) noexcept;
    void draw(const lofibox::tui::TuiFrame& next);
    void forceFullRepaint();

private:
    TerminalPresenter& presenter_;
    std::optional<lofibox::tui::TuiFrame> previous_{};
    bool force_full_{true};
};

} // namespace lofibox::platform::tui

