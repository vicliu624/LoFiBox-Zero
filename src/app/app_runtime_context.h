// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <filesystem>
#include <vector>

#include "app/app_command_executor.h"
#include "app/app_debug_snapshot.h"
#include "app/app_input_router.h"
#include "app/app_lifecycle.h"
#include "app/app_renderer.h"
#include "app/app_state.h"
#include "app/library_controller.h"
#include "app/navigation_state.h"
#include "app/playback_controller.h"
#include "app/runtime_services.h"
#include "core/canvas.h"
#include "ui/ui_models.h"

namespace lofibox::app {

class AppRuntimeContext final : public AppInputTarget,
                                public AppRenderTarget,
                                public AppLifecycleTarget,
                                public AppCommandTarget {
public:
    explicit AppRuntimeContext(std::vector<std::filesystem::path> media_roots = {},
                               ui::UiAssets assets = {},
                               RuntimeServices services = {});

    void update();
    void handleInput(const InputEvent& event);
    void render(core::Canvas& canvas) const;
    [[nodiscard]] AppDebugSnapshot snapshot() const;

    [[nodiscard]] bool bootAnimationComplete() const override;
    void refreshRuntimeStatusIfDue() override;
    [[nodiscard]] AppPage currentPage() const noexcept override;
    NavigationState& navigationState() noexcept override;
    LibraryController& libraryController() noexcept override;
    PlaybackController& playbackController() noexcept override;
    EqState& eqState() noexcept override;
    int& mainMenuIndex() noexcept override;
    void closeHelpForCommand() noexcept override;
    [[nodiscard]] std::chrono::steady_clock::time_point lastUpdate() const noexcept override;
    void setLastUpdate(std::chrono::steady_clock::time_point value) noexcept override;
    [[nodiscard]] const ui::UiAssets& assets() const noexcept override;
    [[nodiscard]] std::chrono::steady_clock::time_point bootStarted() const noexcept override;
    [[nodiscard]] LibraryIndexState libraryState() const noexcept override;
    void startLibraryLoading() override;
    void refreshLibrary() override;
    void showMainMenu() override;
    void updatePlayback(double delta_seconds) override;
    [[nodiscard]] StorageInfo storage() const override;
    [[nodiscard]] bool networkConnected() const noexcept override;
    [[nodiscard]] int mainMenuIndex() const noexcept override;
    [[nodiscard]] const PlaybackSession& playbackSession() const noexcept override;
    [[nodiscard]] const EqState& eqState() const noexcept override;
    [[nodiscard]] ListSelection listSelection() const noexcept override;
    [[nodiscard]] AppPage helpPage() const noexcept override;
    [[nodiscard]] const TrackRecord* findTrack(int id) const noexcept override;
    void pushPage(AppPage page) override;
    void popPage() override;
    void closeHelp() noexcept override;
    void toggleHelpForCurrentPage() noexcept override;
    [[nodiscard]] bool helpOpen() const noexcept override;
    void playFromMenu() override;
    void pausePlayback() override;
    void stepTrack(int delta) override;
    void cycleMainMenuPlaybackMode() override;
    [[nodiscard]] AppPageModel pageModel() const override;
    void moveMainMenuSelection(int delta) override;
    void resetMainMenuSelection() noexcept override;
    void moveSelection(int delta) override;
    [[nodiscard]] bool isBrowseListPage() const noexcept override;
    [[nodiscard]] bool nowPlayingConfirmBlocked() const override;
    void confirmMainMenu() override;
    void toggleShuffle() override;
    void cycleRepeatMode() override;
    void togglePlayPause() override;
    void moveEqualizerSelection(int delta) override;
    void adjustSelectedEqualizerBand(int delta) override;
    void cycleSongSortModeAndClamp() override;
    void confirmListPage() override;

private:
    using clock = std::chrono::steady_clock;

    void refreshNetworkStatus();
    void refreshMetadataServiceState();
    void refreshRuntimeStatus(bool force);

    std::vector<std::filesystem::path> media_roots_{};
    ui::UiAssets ui_assets_{};
    LibraryController library_controller_{};
    clock::time_point boot_started_{clock::now()};
    NavigationState navigation_{};
    int main_menu_index_{1};
    SettingsState settings_{};
    NetworkState network_{};
    MetadataServiceState metadata_service_{};
    EqState eq_{};
    PlaybackController playback_controller_{};
    RuntimeServices services_{};
    clock::time_point last_update_{clock::now()};
    clock::time_point last_status_refresh_{};
    clock::time_point now_playing_confirm_blocked_until_{};
    bool help_open_{false};
    AppPage help_page_{AppPage::MainMenu};
};

} // namespace lofibox::app
