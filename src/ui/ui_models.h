// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <optional>
#include <string>

#include "core/canvas.h"

namespace lofibox::ui {

struct UiAssets {
    std::optional<core::Canvas> logo{};
    std::optional<core::Canvas> music_icon{};
    std::optional<core::Canvas> library_icon{};
    std::optional<core::Canvas> playlists_icon{};
    std::optional<core::Canvas> now_playing_icon{};
    std::optional<core::Canvas> equalizer_icon{};
    std::optional<core::Canvas> settings_icon{};
};

struct SpectrumFrame {
    bool available{false};
    std::array<float, 10> bands{};
};

struct LyricsContent {
    std::optional<std::string> plain{};
    std::optional<std::string> synced{};
};

} // namespace lofibox::ui
