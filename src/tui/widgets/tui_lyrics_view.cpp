// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/widgets/tui_lyrics_view.h"

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::widgets {

std::vector<std::string> renderLyricsView(const runtime::LyricsRuntimeSnapshot& lyrics, int max_lines, int width)
{
    std::vector<std::string> lines{};
    if (max_lines <= 0) {
        return lines;
    }
    if (!lyrics.available || lyrics.visible_lines.empty()) {
        lines.push_back("Lyrics unavailable");
        lines.push_back(fitText(lyrics.status_message.empty() ? "source: none" : lyrics.status_message, width));
        return lines;
    }
    for (const auto& line : lyrics.visible_lines) {
        if (static_cast<int>(lines.size()) >= max_lines) {
            break;
        }
        lines.push_back(fitText(std::string{line.current ? "> " : "  "} + line.text, width));
    }
    return lines;
}

} // namespace lofibox::tui::widgets

