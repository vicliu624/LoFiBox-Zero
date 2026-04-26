// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include "core/canvas.h"

namespace lofibox::app {

struct AppAssets {
    std::optional<core::Canvas> logo{};
    std::optional<core::Canvas> music_icon{};
    std::optional<core::Canvas> library_icon{};
    std::optional<core::Canvas> playlists_icon{};
    std::optional<core::Canvas> now_playing_icon{};
    std::optional<core::Canvas> equalizer_icon{};
    std::optional<core::Canvas> settings_icon{};
};

} // namespace lofibox::app
