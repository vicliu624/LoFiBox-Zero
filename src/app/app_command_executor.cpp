// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_command_executor.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace lofibox::app {
namespace {

constexpr std::array<AppPage, 6> kMainMenuPages{
    AppPage::Songs,
    AppPage::MusicIndex,
    AppPage::Playlists,
    AppPage::NowPlaying,
    AppPage::Equalizer,
    AppPage::Settings,
};
constexpr int kEqMinGainDb = -12;
constexpr int kEqMaxGainDb = 12;
constexpr int kSettingsRemoteMediaIndex = 5;
constexpr int kSettingsAboutIndex = 6;

void clampListSelection(AppCommandTarget& target)
{
    const auto model = target.pageModel();
    target.navigationState().clampListSelection(static_cast<int>(model.rows.size()), model.max_visible_rows);
}

} // namespace

void commandPushPage(AppCommandTarget& target, AppPage page)
{
    target.closeHelpForCommand();
    target.navigationState().pushPage(page);
}

void commandPopPage(AppCommandTarget& target)
{
    target.closeHelpForCommand();
    target.navigationState().popPage();
}

void commandPlayFromMenu(AppCommandTarget& target)
{
    auto& playback_controller = target.playbackController();
    auto& library_controller = target.libraryController();
    const auto& session = playback_controller.session();
    if (session.status == PlaybackStatus::Paused && session.current_track_id) {
        playback_controller.resume();
        return;
    }
    if (session.status == PlaybackStatus::Playing) {
        return;
    }
    if (session.current_track_id) {
        playback_controller.resume();
        return;
    }

    const auto ids = library_controller.allSongIdsSorted();
    if (!ids.empty()) {
        library_controller.setSongsContextAll();
        (void)playback_controller.startTrack(library_controller, ids.front());
    }
}

void commandPausePlayback(AppCommandTarget& target)
{
    target.playbackController().pause();
}

void commandStepTrack(AppCommandTarget& target, int delta)
{
    target.playbackController().stepQueue(target.libraryController(), delta);
}

void commandCycleMainMenuPlaybackMode(AppCommandTarget& target)
{
    auto& playback_controller = target.playbackController();
    const auto& session = playback_controller.session();
    if (session.shuffle_enabled) {
        playback_controller.setShuffleEnabled(false);
        playback_controller.setRepeatOne(true);
        return;
    }
    if (session.repeat_one) {
        playback_controller.setRepeatOne(false);
        playback_controller.setRepeatAll(false);
        return;
    }
    playback_controller.setRepeatOne(false);
    playback_controller.setRepeatAll(false);
    playback_controller.setShuffleEnabled(true);
}

void commandMoveMainMenuSelection(AppCommandTarget& target, int delta)
{
    auto& index = target.mainMenuIndex();
    index = (index + static_cast<int>(kMainMenuPages.size()) + delta) % static_cast<int>(kMainMenuPages.size());
}

void commandResetMainMenuSelection(AppCommandTarget& target) noexcept
{
    target.mainMenuIndex() = 0;
}

void commandConfirmMainMenu(AppCommandTarget& target)
{
    const int index = target.mainMenuIndex();
    if (index < 0 || index >= static_cast<int>(kMainMenuPages.size())) {
        return;
    }
    if (kMainMenuPages[static_cast<std::size_t>(index)] == AppPage::Songs) {
        target.libraryController().setSongsContextAll();
    }
    commandPushPage(target, kMainMenuPages[static_cast<std::size_t>(index)]);
}

void commandToggleShuffle(AppCommandTarget& target)
{
    target.playbackController().toggleShuffle();
}

void commandCycleRepeatMode(AppCommandTarget& target)
{
    target.playbackController().cycleRepeatMode();
}

void commandTogglePlayPause(AppCommandTarget& target)
{
    target.playbackController().togglePlayPause();
}

void commandMoveEqualizerSelection(AppCommandTarget& target, int delta)
{
    auto& eq = target.eqState();
    eq.selected_band = std::clamp(eq.selected_band + delta, 0, static_cast<int>(eq.bands.size()) - 1);
}

void commandAdjustSelectedEqualizerBand(AppCommandTarget& target, int delta)
{
    auto& eq = target.eqState();
    auto& band = eq.bands[static_cast<std::size_t>(eq.selected_band)];
    band = std::clamp(band + delta, kEqMinGainDb, kEqMaxGainDb);
}

void commandCycleSongSortModeAndClamp(AppCommandTarget& target)
{
    target.libraryController().cycleSongSortMode();
    clampListSelection(target);
}

void commandMoveSelection(AppCommandTarget& target, int delta)
{
    const auto model = target.pageModel();
    target.navigationState().moveSelection(delta, static_cast<int>(model.rows.size()), model.max_visible_rows);
}

void commandConfirmListPage(AppCommandTarget& target)
{
    const int selected = target.navigationState().list_selection.selected;
    const auto page = target.currentPage();
    auto& library_controller = target.libraryController();
    auto& playback_controller = target.playbackController();
    const auto library_result = library_controller.openSelectedListItem(page, selected);
    switch (library_result.kind) {
    case LibraryOpenResult::Kind::PushPage:
        commandPushPage(target, library_result.page);
        return;
    case LibraryOpenResult::Kind::StartTrack:
        if (playback_controller.startTrack(library_controller, library_result.track_id) && target.currentPage() != AppPage::NowPlaying) {
            commandPushPage(target, AppPage::NowPlaying);
        }
        return;
    case LibraryOpenResult::Kind::None:
        break;
    }

    if (page == AppPage::Settings && selected == kSettingsRemoteMediaIndex) {
        commandPushPage(target, AppPage::SourceManager);
        return;
    }

    if (page == AppPage::Settings && selected == kSettingsAboutIndex) {
        commandPushPage(target, AppPage::About);
    }
}

} // namespace lofibox::app
