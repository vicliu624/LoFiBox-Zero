// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/tui/terminal_presenter.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "platform/tui/ansi_escape.h"
#include "platform/tui/terminal_capabilities.h"

#if defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace lofibox::platform::tui {

TerminalPresenter::TerminalPresenter(std::ostream& output)
    : output_(output)
{
}

lofibox::tui::TerminalSize TerminalPresenter::size() const
{
#if defined(__unix__) || defined(__APPLE__)
    winsize ws{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        return lofibox::tui::TerminalSize{static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row)};
    }
#endif
    const int columns = std::getenv("COLUMNS") ? std::atoi(std::getenv("COLUMNS")) : 80;
    const int rows = std::getenv("LINES") ? std::atoi(std::getenv("LINES")) : 24;
    return lofibox::tui::TerminalSize{columns > 0 ? columns : 80, rows > 0 ? rows : 24};
}

void TerminalPresenter::enterAlternateScreen() { output_ << ::lofibox::platform::tui::enterAlternateScreen(); }
void TerminalPresenter::leaveAlternateScreen() { output_ << ::lofibox::platform::tui::leaveAlternateScreen(); }
void TerminalPresenter::hideCursor() { output_ << ::lofibox::platform::tui::hideCursor(); }
void TerminalPresenter::showCursor() { output_ << ::lofibox::platform::tui::showCursor(); }
void TerminalPresenter::clear() { output_ << ::lofibox::platform::tui::clearScreen(); }
void TerminalPresenter::moveCursor(int row, int col) { output_ << ::lofibox::platform::tui::moveCursor(row, col); }
void TerminalPresenter::write(std::string_view text) { output_ << text; }
void TerminalPresenter::flush() { output_.flush(); }
bool TerminalPresenter::colorSupported() const { return detectTerminalCapabilities().color; }
bool TerminalPresenter::unicodeSupported() const { return detectTerminalCapabilities().unicode; }

TerminalDiffRenderer::TerminalDiffRenderer(TerminalPresenter& presenter) noexcept
    : presenter_(presenter)
{
}

void TerminalDiffRenderer::forceFullRepaint()
{
    force_full_ = true;
}

void TerminalDiffRenderer::draw(const lofibox::tui::TuiFrame& next)
{
    if (!previous_ || force_full_ || previous_->width != next.width || previous_->height != next.height) {
        presenter_.clear();
        for (int row = 0; row < next.height; ++row) {
            presenter_.moveCursor(row, 0);
            presenter_.write(next.line(row));
        }
        previous_ = next;
        force_full_ = false;
        presenter_.flush();
        return;
    }
    for (int row = 0; row < next.height; ++row) {
        const auto current = next.line(row);
        if (current == previous_->line(row)) {
            continue;
        }
        presenter_.moveCursor(row, 0);
        presenter_.write(current);
        presenter_.write(std::string(static_cast<std::size_t>(std::max(0, next.width - static_cast<int>(current.size()))), ' '));
    }
    previous_ = next;
    presenter_.flush();
}

} // namespace lofibox::platform::tui
