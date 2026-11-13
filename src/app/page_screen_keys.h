// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string_view>

#include "app/app_page.h"
#include "core/display_profile.h"

namespace lofibox::app {

enum class ScreenKeyAction {
    None, Back, MenuPrevious, MenuOpen, MenuNext, NowPlaying, Search,
    PageUp, Open, PlaySelection, PageDown, Sort, PlaybackMode,
    PreviousTrack, PlayPause, NextTrack, Lyrics, Queue,
    PreviousBand, GainDown, Preset, GainUp, NextBand, Library, Settings, Help,
};

struct ScreenKey {
    ScreenKeyAction action{ScreenKeyAction::None};
    std::string_view label{};
    std::string_view description{};
};

inline constexpr std::array<int, 5> kScreenKeyCenters{49, 105, 161, 217, 273};
inline constexpr int kScreenKeyBarTop = core::kScreenKeyBarTop;

// The same immutable definition drives input, pointer hit testing and hints.
// Text-entry pages deliberately have no numeric actions.
[[nodiscard]] constexpr std::array<ScreenKey, 5> screenKeysForPage(AppPage page) noexcept
{
    using A = ScreenKeyAction;
    switch (page) {
    case AppPage::Boot:
    case AppPage::Search:
    case AppPage::RemoteFieldEditor:
        return {};
    case AppPage::MainMenu:
        return {{{A::MenuPrevious, "LEFT", "PREVIOUS MENU ITEM"},
                 {A::MenuOpen, "OPEN", "OPEN MENU ITEM"},
                 {A::MenuNext, "RIGHT", "NEXT MENU ITEM"},
                 {A::NowPlaying, "PLAYER", "NOW PLAYING"},
                 {A::Search, "SEARCH", "SEARCH MUSIC"}}};
    case AppPage::NowPlaying:
        return {{{A::PreviousTrack, "PREV", "PREVIOUS TRACK"},
                 {A::PlayPause, "PLAY", "PLAY / PAUSE"},
                 {A::NextTrack, "NEXT", "NEXT TRACK"},
                 {A::Lyrics, "LYRICS", "OPEN LYRICS"},
                 {A::Queue, "QUEUE", "OPEN QUEUE"}}};
    case AppPage::Lyrics:
        return {{{A::PreviousTrack, "PREV", "PREVIOUS TRACK"},
                 {A::PlayPause, "PLAY", "PLAY / PAUSE"},
                 {A::NextTrack, "NEXT", "NEXT TRACK"},
                 {A::NowPlaying, "PLAYER", "NOW PLAYING"},
                 {A::PlaybackMode, "MODE", "CYCLE PLAY MODE"}}};
    case AppPage::Equalizer:
        return {{{A::PreviousBand, "BAND-", "PREVIOUS BAND"},
                 {A::GainDown, "GAIN-", "GAIN -1DB"},
                 {A::Preset, "PRESET", "NEXT PRESET"},
                 {A::GainUp, "GAIN+", "GAIN +1DB"},
                 {A::NextBand, "BAND+", "NEXT BAND"}}};
    case AppPage::Songs:
        return {{{A::Back, "BACK", "BACK"}, {A::PageUp, "PG UP", "PAGE UP"},
                 {A::PlaySelection, "PLAY", "PLAY SELECTED TRACK"},
                 {A::PageDown, "PG DN", "PAGE DOWN"}, {A::Sort, "SORT", "CYCLE SORT ORDER"}}};
    case AppPage::Playlists:
    case AppPage::PlaylistDetail:
        return {{{A::Back, "BACK", "BACK"}, {A::PageUp, "PG UP", "PAGE UP"},
                 {page == AppPage::PlaylistDetail ? A::PlaySelection : A::Open,
                  page == AppPage::PlaylistDetail ? "PLAY" : "OPEN", "OPEN / PLAY SELECTION"},
                 {A::PageDown, "PG DN", "PAGE DOWN"},
                 {page == AppPage::PlaylistDetail ? A::Sort : A::Search,
                  page == AppPage::PlaylistDetail ? "SORT" : "SEARCH", "SORT / SEARCH MUSIC"}}};
    case AppPage::Queue:
        return {{{A::Back, "BACK", "BACK"},
                 {A::PageUp, "PG UP", "PAGE UP"},
                 {A::PlaySelection, "PLAY", "PLAY QUEUED SELECTION"},
                 {A::PageDown, "PG DN", "PAGE DOWN"},
                 {A::PlaybackMode, "MODE", "CYCLE PLAY MODE"}}};
    case AppPage::StreamDetail:
        return {{{A::Back, "BACK", "BACK"}, {}, {A::PlaySelection, "PLAY", "PLAY STREAM"},
                 {}, {A::Help, "HELP", "PAGE HELP"}}};
    case AppPage::About:
        return {{{A::Back, "BACK", "BACK"}, {A::Library, "LIBRARY", "MUSIC LIBRARY"},
                 {A::NowPlaying, "PLAYER", "NOW PLAYING"},
                 {A::Settings, "SETUP", "SETTINGS"}, {A::Help, "HELP", "PAGE HELP"}}};
    case AppPage::ServerDiagnostics:
        return {{{A::Back, "BACK", "BACK"}, {A::PageUp, "PG UP", "PAGE UP"},
                 {A::Help, "HELP", "PAGE HELP"}, {A::PageDown, "PG DN", "PAGE DOWN"},
                 {A::Settings, "SETUP", "SETTINGS"}}};
    case AppPage::PlaylistEditor:
        // The current editor has no confirmed destructive keyboard operation.
        return {{{A::Back, "BACK", "BACK"}, {A::PageUp, "PG UP", "PAGE UP"},
                 {A::Help, "HELP", "PAGE HELP"}, {A::PageDown, "PG DN", "PAGE DOWN"},
                 {A::Search, "SEARCH", "SEARCH MUSIC"}}};
    case AppPage::MusicIndex:
    case AppPage::Artists:
    case AppPage::Albums:
    case AppPage::Genres:
    case AppPage::Composers:
    case AppPage::Compilations:
    case AppPage::RemoteBrowse:
        return {{{A::Back, "BACK", "BACK"}, {A::PageUp, "PG UP", "PAGE UP"},
                 {A::Open, "OPEN", "OPEN SELECTION"}, {A::PageDown, "PG DN", "PAGE DOWN"},
                 {A::Search, "SEARCH", "SEARCH MUSIC"}}};
    case AppPage::Settings:
    case AppPage::RemoteSetup:
    case AppPage::SourceManager:
    case AppPage::RemoteProfileSettings:
        return {{{A::Back, "BACK", "BACK"}, {A::PageUp, "PG UP", "PAGE UP"},
                 {A::Open, "OPEN", "OPEN / CHANGE SELECTION"},
                 {A::PageDown, "PG DN", "PAGE DOWN"}, {A::Help, "HELP", "PAGE HELP"}}};
    }
    return {};
}

} // namespace lofibox::app
