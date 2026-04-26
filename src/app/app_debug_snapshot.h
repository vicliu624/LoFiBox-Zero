// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <optional>

#include "app/app_page.h"
#include "playback/playback_state.h"

namespace lofibox::app {

struct AppDebugSnapshot {
    AppPage current_page{AppPage::Boot};
    bool library_ready{false};
    std::size_t track_count{0};
    std::size_t visible_count{0};
    int list_selected_index{0};
    int list_scroll_offset{0};
    bool playback_active{false};
    PlaybackStatus playback_status{PlaybackStatus::Empty};
    std::optional<int> current_track_id{};
    bool shuffle_enabled{false};
    bool repeat_all{false};
    bool repeat_one{false};
    bool network_connected{false};
    int eq_band_count{0};
    int eq_selected_band{0};
};

} // namespace lofibox::app

