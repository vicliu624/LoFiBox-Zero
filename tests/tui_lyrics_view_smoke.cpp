// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "tui/widgets/tui_lyrics_view.h"

int main()
{
    lofibox::runtime::LyricsRuntimeSnapshot empty{};
    auto lines = lofibox::tui::widgets::renderLyricsView(empty, 3, 40);
    if (lines.empty() || lines.front().find("unavailable") == std::string::npos) {
        std::cerr << "Expected no-lyrics fallback.\n";
        return 1;
    }

    lofibox::runtime::LyricsRuntimeSnapshot lyrics{};
    lyrics.available = true;
    lyrics.current_index = 1;
    lyrics.visible_lines.push_back(lofibox::runtime::RuntimeLyricLine{0, 0.0, "first", "", false});
    lyrics.visible_lines.push_back(lofibox::runtime::RuntimeLyricLine{1, 1.0, "current", "", true});
    lines = lofibox::tui::widgets::renderLyricsView(lyrics, 3, 40);
    if (lines.size() < 2 || lines[1].find("> current") == std::string::npos) {
        std::cerr << "Expected current lyric line marker.\n";
        return 1;
    }
    return 0;
}

