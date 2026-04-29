// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

namespace lofibox::tui {

enum class TuiViewMode {
    Dashboard,
    NowPlaying,
    Lyrics,
    Spectrum,
    Queue,
    Library,
    Sources,
    Eq,
    Dsp,
    Diagnostics,
    Creator,
    Help,
    CommandPalette,
};

[[nodiscard]] TuiViewMode tuiViewModeFromName(std::string_view name) noexcept;
[[nodiscard]] std::string tuiViewModeName(TuiViewMode mode);
[[nodiscard]] TuiViewMode nextTuiViewMode(TuiViewMode mode) noexcept;
[[nodiscard]] TuiViewMode previousTuiViewMode(TuiViewMode mode) noexcept;

} // namespace lofibox::tui

