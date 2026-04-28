// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_command_executor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

#include "audio/dsp/dsp_chain.h"

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
constexpr int kSettingsRemoteSetupIndex = 5;
constexpr int kSettingsAboutIndex = 6;

std::string upperAscii(std::string_view text)
{
    std::string result{text};
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

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
    (void)target.appServices().playbackCommands().playFirstAvailable();
}

void commandPausePlayback(AppCommandTarget& target)
{
    target.appServices().playbackCommands().pause();
}

void commandStepTrack(AppCommandTarget& target, int delta)
{
    target.appServices().queueCommands().step(delta);
}

void commandCycleMainMenuPlaybackMode(AppCommandTarget& target)
{
    target.appServices().playbackCommands().cycleMainMenuPlaybackMode();
}

void commandToggleRepeatAll(AppCommandTarget& target)
{
    auto playback = target.appServices().playbackCommands();
    const bool enabled = !playback.session().repeat_all;
    playback.setRepeatAll(enabled);
    if (!enabled) {
        playback.setRepeatOne(false);
    }
}

void commandToggleRepeatOne(AppCommandTarget& target)
{
    auto playback = target.appServices().playbackCommands();
    const bool enabled = !playback.session().repeat_one;
    playback.setRepeatOne(enabled);
    if (!enabled) {
        playback.setRepeatAll(false);
    }
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
        target.appServices().libraryMutations().setSongsContextAll();
    }
    commandPushPage(target, kMainMenuPages[static_cast<std::size_t>(index)]);
}

void commandToggleShuffle(AppCommandTarget& target)
{
    target.appServices().playbackCommands().toggleShuffle();
}

void commandCycleRepeatMode(AppCommandTarget& target)
{
    target.appServices().playbackCommands().cycleRepeatMode();
}

void commandTogglePlayPause(AppCommandTarget& target)
{
    target.appServices().playbackCommands().togglePlayPause();
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
    eq.preset_name = "CUSTOM";
}

void commandCycleEqualizerPreset(AppCommandTarget& target, int delta)
{
    auto presets = audio::dsp::builtinEqPresets();
    if (presets.empty()) {
        return;
    }

    auto& eq = target.eqState();
    int current = -1;
    const auto current_name = upperAscii(eq.preset_name);
    for (int index = 0; index < static_cast<int>(presets.size()); ++index) {
        if (upperAscii(presets[static_cast<std::size_t>(index)].name) == current_name) {
            current = index;
            break;
        }
    }

    const int count = static_cast<int>(presets.size());
    const int next = ((current + delta) % count + count) % count;
    const auto& preset = presets[static_cast<std::size_t>(next)];
    for (std::size_t index = 0; index < eq.bands.size() && index < preset.bands.size(); ++index) {
        eq.bands[index] = std::clamp(static_cast<int>(std::round(preset.bands[index].gain_db)), kEqMinGainDb, kEqMaxGainDb);
    }
    eq.preset_name = upperAscii(preset.name);
}

void commandCycleSongSortModeAndClamp(AppCommandTarget& target)
{
    target.appServices().libraryMutations().cycleSongSortMode();
    clampListSelection(target);
}

void commandMoveSelection(AppCommandTarget& target, int delta)
{
    const auto model = target.pageModel();
    target.navigationState().moveSelection(delta, static_cast<int>(model.rows.size()), model.max_visible_rows);
}

void commandMoveSelectionPage(AppCommandTarget& target, int delta_pages)
{
    const auto model = target.pageModel();
    const int count = static_cast<int>(model.rows.size());
    auto& navigation = target.navigationState();
    if (count <= 0) {
        navigation.list_selection = {};
        return;
    }
    const int step = std::max(1, model.max_visible_rows - 1);
    navigation.list_selection.selected = std::clamp(
        navigation.list_selection.selected + (delta_pages * step),
        0,
        count - 1);
    navigation.clampListSelection(count, model.max_visible_rows);
}

void commandConfirmListPage(AppCommandTarget& target)
{
    const int selected = target.navigationState().list_selection.selected;
    const auto page = target.currentPage();
    const auto library_result = target.appServices().libraryOpenActions().openSelectedListItem(page, selected);
    switch (library_result.kind) {
    case LibraryOpenResult::Kind::PushPage:
        commandPushPage(target, library_result.page);
        return;
    case LibraryOpenResult::Kind::StartTrack:
        if (target.startLibraryTrack(library_result.track_id) && target.currentPage() != AppPage::NowPlaying) {
            commandPushPage(target, AppPage::NowPlaying);
        }
        return;
    case LibraryOpenResult::Kind::None:
        break;
    }

    if (page == AppPage::MusicIndex && target.handleLibraryRemoteConfirm(selected)) {
        return;
    }

    if (page == AppPage::Settings && selected == kSettingsRemoteSetupIndex) {
        (void)target.handleSettingsRemoteConfirm(selected);
        return;
    }

    if (page == AppPage::RemoteSetup) {
        (void)target.handleRemoteSetupConfirm(selected);
        return;
    }

    if (page == AppPage::SourceManager) {
        if (!target.handleSourceManagerConfirm(selected)) {
            commandPushPage(target, selected == 0 ? AppPage::MusicIndex : AppPage::ServerDiagnostics);
        }
        return;
    }

    if (page == AppPage::RemoteProfileSettings) {
        (void)target.handleRemoteProfileSettingsConfirm(selected);
        return;
    }

    if (page == AppPage::RemoteBrowse && selected >= 0) {
        (void)target.handleRemoteBrowseConfirm(selected);
        return;
    }

    if (page == AppPage::StreamDetail) {
        if (target.handleStreamDetailConfirm()) {
            commandPushPage(target, AppPage::NowPlaying);
        }
        return;
    }

    if (page == AppPage::Search) {
        if (target.handleSearchConfirm(selected)) {
            commandPushPage(target, AppPage::NowPlaying);
        }
        return;
    }

    if (page == AppPage::Settings && selected == kSettingsAboutIndex) {
        commandPushPage(target, AppPage::About);
    }
}

} // namespace lofibox::app
