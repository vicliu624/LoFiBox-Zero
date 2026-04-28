// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/app_page.h"
#include "app/app_page_model.h"
#include "app/app_state.h"
#include "app/library_controller.h"
#include "app/navigation_state.h"
#include "playback/playback_controller.h"

namespace lofibox::app {

class AppCommandTarget {
public:
    virtual ~AppCommandTarget() = default;

    [[nodiscard]] virtual AppPage currentPage() const noexcept = 0;
    [[nodiscard]] virtual AppPageModel pageModel() const = 0;
    virtual NavigationState& navigationState() noexcept = 0;
    virtual LibraryController& libraryController() noexcept = 0;
    virtual PlaybackController& playbackController() noexcept = 0;
    virtual EqState& eqState() noexcept = 0;
    virtual int& mainMenuIndex() noexcept = 0;
    virtual void closeHelpForCommand() noexcept = 0;
    [[nodiscard]] virtual bool startLibraryTrack(int track_id)
    {
        return playbackController().startTrack(libraryController(), track_id);
    }
    virtual bool handleSettingsRemoteConfirm(int selected) { (void)selected; return false; }
    virtual bool handleRemoteSetupConfirm(int selected) { (void)selected; return false; }
    virtual bool handleLibraryRemoteConfirm(int selected) { (void)selected; return false; }
    virtual bool handleSourceManagerConfirm(int selected) { (void)selected; return false; }
    virtual bool handleRemoteProfileSettingsConfirm(int selected) { (void)selected; return false; }
    virtual bool handleRemoteBrowseConfirm(int selected) { (void)selected; return false; }
    virtual bool handleStreamDetailConfirm() { return false; }
    virtual bool handleSearchConfirm(int selected) { (void)selected; return false; }
};

void commandPushPage(AppCommandTarget& target, AppPage page);
void commandPopPage(AppCommandTarget& target);
void commandPlayFromMenu(AppCommandTarget& target);
void commandPausePlayback(AppCommandTarget& target);
void commandStepTrack(AppCommandTarget& target, int delta);
void commandCycleMainMenuPlaybackMode(AppCommandTarget& target);
void commandToggleRepeatAll(AppCommandTarget& target);
void commandToggleRepeatOne(AppCommandTarget& target);
void commandMoveMainMenuSelection(AppCommandTarget& target, int delta);
void commandResetMainMenuSelection(AppCommandTarget& target) noexcept;
void commandConfirmMainMenu(AppCommandTarget& target);
void commandToggleShuffle(AppCommandTarget& target);
void commandCycleRepeatMode(AppCommandTarget& target);
void commandTogglePlayPause(AppCommandTarget& target);
void commandMoveEqualizerSelection(AppCommandTarget& target, int delta);
void commandAdjustSelectedEqualizerBand(AppCommandTarget& target, int delta);
void commandCycleEqualizerPreset(AppCommandTarget& target, int delta);
void commandCycleSongSortModeAndClamp(AppCommandTarget& target);
void commandMoveSelection(AppCommandTarget& target, int delta);
void commandMoveSelectionPage(AppCommandTarget& target, int delta_pages);
void commandConfirmListPage(AppCommandTarget& target);

} // namespace lofibox::app

