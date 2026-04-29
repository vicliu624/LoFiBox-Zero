// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/now_playing_tui_page.h"

#include "tui/widgets/tui_progress_bar.h"
#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {

std::vector<std::string> nowPlayingPageLines(const TuiModel& model, const TuiLayout& layout)
{
    const auto& playback = model.snapshot.playback;
    return {
        widgets::fitText("Now Playing", layout.size.width),
        widgets::fitText(playback.title.empty() ? "No track loaded" : playback.title, layout.size.width),
        widgets::fitText(playback.artist.empty() ? playback.album : playback.artist, layout.size.width),
        widgets::renderProgressLine(playback.elapsed_seconds, playback.duration_seconds, playback.live, layout.size.width, model.options.charset),
        widgets::fitText("source: " + (playback.source_label.empty() ? std::string{"-"} : playback.source_label), layout.size.width),
    };
}

} // namespace lofibox::tui::pages

