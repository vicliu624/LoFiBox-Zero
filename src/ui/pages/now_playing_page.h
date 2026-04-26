// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <optional>
#include <string>

#include "app/audio_visualization.h"
#include "core/canvas.h"

namespace lofibox::ui::pages {

enum class NowPlayingStatus {
    Empty,
    Paused,
    Playing,
};

struct NowPlayingView {
    bool has_track{false};
    std::string title{};
    std::string artist{};
    std::string album{};
    int duration_seconds{0};
    double elapsed_seconds{0.0};
    NowPlayingStatus status{NowPlayingStatus::Empty};
    bool shuffle_enabled{false};
    bool repeat_all{false};
    bool repeat_one{false};
    const core::Canvas* artwork{};
    app::AudioVisualizationFrame visualization{};
};

void renderNowPlayingPage(core::Canvas& canvas, const NowPlayingView& view);

} // namespace lofibox::ui::pages
