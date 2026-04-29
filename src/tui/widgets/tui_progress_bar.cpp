// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/widgets/tui_progress_bar.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace lofibox::tui::widgets {

std::string formatDuration(double seconds)
{
    const int total = std::max(0, static_cast<int>(seconds));
    const int minutes = total / 60;
    const int secs = total % 60;
    std::ostringstream out{};
    out << minutes << ':' << std::setw(2) << std::setfill('0') << secs;
    return out.str();
}

std::string renderProgressBar(double elapsed_seconds, int duration_seconds, bool live, int width, TuiCharset charset)
{
    if (width <= 0) {
        return {};
    }
    const std::string filled = charset == TuiCharset::Ascii ? "#" : (charset == TuiCharset::Minimal ? "=" : "━");
    const std::string empty = charset == TuiCharset::Ascii ? "-" : (charset == TuiCharset::Minimal ? "-" : "─");
    if (live) {
        std::string result{};
        for (int index = 0; index < width; ++index) {
            result += filled;
        }
        return result;
    }

    const double denominator = static_cast<double>(std::max(1, duration_seconds));
    const int fill = std::clamp(static_cast<int>((std::max(0.0, elapsed_seconds) / denominator) * static_cast<double>(width)), 0, width);
    std::string result{};
    for (int index = 0; index < width; ++index) {
        result += index < fill ? filled : empty;
    }
    return result;
}

std::string renderProgressLine(double elapsed_seconds, int duration_seconds, bool live, int width, TuiCharset charset)
{
    if (width <= 0) {
        return {};
    }
    if (live) {
        const int bar_width = std::max(4, width - 18);
        return "LIVE " + renderProgressBar(elapsed_seconds, duration_seconds, true, bar_width, charset) + " radio";
    }
    const auto left = formatDuration(elapsed_seconds);
    const auto right = formatDuration(duration_seconds);
    const int bar_width = std::max(1, width - static_cast<int>(left.size() + right.size()) - 2);
    return left + " " + renderProgressBar(elapsed_seconds, duration_seconds, false, bar_width, charset) + " " + right;
}

} // namespace lofibox::tui::widgets

