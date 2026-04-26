// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "app/audio_visualization.h"
#include "ui/pages/now_playing_page.h"
#include "app/runtime_services.h"
#include "core/canvas.h"

namespace lofibox::ui::pages {

struct LyricsPageView {
    bool has_track{false};
    std::string title{};
    std::string artist{};
    int duration_seconds{0};
    double elapsed_seconds{0.0};
    NowPlayingStatus status{NowPlayingStatus::Empty};
    bool lookup_pending{false};
    app::TrackLyrics lyrics{};
    app::AudioVisualizationFrame visualization{};
};

void renderLyricsPage(core::Canvas& canvas, const LyricsPageView& view);

} // namespace lofibox::ui::pages
