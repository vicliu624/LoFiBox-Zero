// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/library_tui_page.h"

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {

std::vector<std::string> libraryPageLines(const TuiModel& model, const TuiLayout& layout)
{
    const auto& library = model.snapshot.library;
    return {
        widgets::fitText("Library · " + library.status, layout.size.width),
        widgets::fitText("tracks: " + std::to_string(library.track_count), layout.size.width),
        widgets::fitText("albums: " + std::to_string(library.album_count), layout.size.width),
        widgets::fitText("artists: " + std::to_string(library.artist_count), layout.size.width),
        widgets::fitText(library.degraded ? "status: degraded" : "status: ok", layout.size.width),
    };
}

} // namespace lofibox::tui::pages

