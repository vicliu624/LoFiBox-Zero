// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/lyrics_tui_page.h"

#include "tui/widgets/tui_lyrics_view.h"
#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {

std::vector<std::string> lyricsPageLines(const TuiModel& model, const TuiLayout& layout)
{
    std::vector<std::string> lines{};
    lines.push_back(widgets::fitText("Lyrics · offset "
        + std::to_string(model.snapshot.lyrics.offset_seconds) + "s", layout.size.width));
    for (const auto& line : widgets::renderLyricsView(model.snapshot.lyrics, layout.size.height - 3, layout.size.width)) {
        lines.push_back(line);
    }
    lines.push_back(widgets::fitText("source: " + (model.snapshot.lyrics.source.empty() ? std::string{"none"} : model.snapshot.lyrics.source), layout.size.width));
    return lines;
}

} // namespace lofibox::tui::pages

