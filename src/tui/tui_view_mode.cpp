// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/tui_view_mode.h"

#include <array>

namespace lofibox::tui {
namespace {

struct ViewName {
    TuiViewMode mode;
    std::string_view name;
};

constexpr std::array<ViewName, 13> kViews{{
    {TuiViewMode::Dashboard, "dashboard"},
    {TuiViewMode::NowPlaying, "now"},
    {TuiViewMode::Lyrics, "lyrics"},
    {TuiViewMode::Spectrum, "spectrum"},
    {TuiViewMode::Queue, "queue"},
    {TuiViewMode::Library, "library"},
    {TuiViewMode::Sources, "sources"},
    {TuiViewMode::Eq, "eq"},
    {TuiViewMode::Dsp, "dsp"},
    {TuiViewMode::Diagnostics, "diagnostics"},
    {TuiViewMode::Creator, "creator"},
    {TuiViewMode::Help, "help"},
    {TuiViewMode::CommandPalette, "command-palette"},
}};

constexpr std::array<TuiViewMode, 10> kCycle{{
    TuiViewMode::Dashboard,
    TuiViewMode::NowPlaying,
    TuiViewMode::Lyrics,
    TuiViewMode::Spectrum,
    TuiViewMode::Queue,
    TuiViewMode::Library,
    TuiViewMode::Sources,
    TuiViewMode::Eq,
    TuiViewMode::Diagnostics,
    TuiViewMode::Help,
}};

int cycleIndex(TuiViewMode mode) noexcept
{
    for (int index = 0; index < static_cast<int>(kCycle.size()); ++index) {
        if (kCycle[static_cast<std::size_t>(index)] == mode) {
            return index;
        }
    }
    return 0;
}

} // namespace

TuiViewMode tuiViewModeFromName(std::string_view name) noexcept
{
    if (name == "tui") {
        return TuiViewMode::Dashboard;
    }
    if (name == "now-playing") {
        return TuiViewMode::NowPlaying;
    }
    for (const auto& view : kViews) {
        if (view.name == name) {
            return view.mode;
        }
    }
    return TuiViewMode::Dashboard;
}

std::string tuiViewModeName(TuiViewMode mode)
{
    for (const auto& view : kViews) {
        if (view.mode == mode) {
            return std::string{view.name};
        }
    }
    return "dashboard";
}

TuiViewMode nextTuiViewMode(TuiViewMode mode) noexcept
{
    const int index = cycleIndex(mode);
    return kCycle[static_cast<std::size_t>((index + 1) % static_cast<int>(kCycle.size()))];
}

TuiViewMode previousTuiViewMode(TuiViewMode mode) noexcept
{
    const int index = cycleIndex(mode);
    const int next = (index - 1 + static_cast<int>(kCycle.size())) % static_cast<int>(kCycle.size());
    return kCycle[static_cast<std::size_t>(next)];
}

} // namespace lofibox::tui

