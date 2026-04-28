// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "app/app_command_executor.h"
#include "app/app_controller_set.h"
#include "app/app_debug_snapshot.h"
#include "app/app_input_router.h"
#include "app/app_lifecycle.h"
#include "app/app_renderer.h"
#include "app/app_runtime_state.h"
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
                               RuntimeServices services = {},
                               std::vector<std::string> startup_uris = {});

    void update();
    void handleInput(const InputEvent& event);
    void render(core::Canvas& canvas) const;
    [[nodiscard]] AppDebugSnapshot snapshot() const;

    [[nodiscard]] bool bootAnimationComplete() const override;
    void refreshRuntimeStatusIfDue() override;
    [[nodiscard]] AppPage currentPage() const noexcept override;
    [[nodiscard]] ::lofibox::application::AppServiceRegistry appServices() noexcept override;
    NavigationState& navigationState() noexcept override;
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
    void toggleRepeatAll() override;
    void toggleRepeatOne() override;
    void openSearchPage() override;
    void openLibraryPage() override;
    void openQueuePage() override;
    void openSettingsPage() override;
    void showMainMenuPage() override;
    [[nodiscard]] AppPageModel pageModel() const override;
    void moveMainMenuSelection(int delta) override;
    void resetMainMenuSelection() noexcept override;
    void moveSelection(int delta) override;
    [[nodiscard]] bool isBrowseListPage() const noexcept override;
    [[nodiscard]] bool nowPlayingConfirmBlocked() const override;
    void confirmMainMenu() override;
    bool startLibraryTrack(int track_id) override;
    void toggleShuffle() override;
    void cycleRepeatMode() override;
    void togglePlayPause() override;
    void moveEqualizerSelection(int delta) override;
    void adjustSelectedEqualizerBand(int delta) override;
    void cycleEqualizerPreset(int delta) override;
    void cycleSongSortModeAndClamp() override;
    void confirmListPage() override;
    void moveSelectionPage(int delta_pages) override;
    bool handleSettingsRemoteConfirm(int selected) override;
    bool handleRemoteSetupConfirm(int selected) override;
    bool handleLibraryRemoteConfirm(int selected) override;
    bool handleSourceManagerConfirm(int selected) override;
    bool handleRemoteProfileSettingsConfirm(int selected) override;
    bool handleRemoteBrowseConfirm(int selected) override;
    bool handleStreamDetailConfirm() override;
    bool handleSearchConfirm(int selected) override;
    void appendSearchText(std::string_view text) override;
    void backspaceSearchQuery() override;
    void appendRemoteProfileEditText(std::string_view text) override;
    void backspaceRemoteProfileEdit() override;
    void commitRemoteProfileEdit() override;

private:
    using clock = std::chrono::steady_clock;

    void refreshNetworkStatus();
    void refreshMetadataServiceState();
    void refreshRuntimeStatus(bool force);
    void refreshRemoteLibraryTracks();
    void handlePendingOpenRequests();
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> currentPageRows() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> libraryIndexRows() const;
    [[nodiscard]] int libraryRemoteRowBase() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> remoteBrowseRows() const;
    [[nodiscard]] std::optional<std::string> remoteBrowseTitleOverride() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> serverDiagnosticsRows() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> streamDetailRows() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> queueRows() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> searchRows() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> remoteSetupRows() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> remoteProfileSettingsRows() const;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> remoteFieldEditorRows() const;
    [[nodiscard]] std::optional<RemoteServerProfile> selectedRemoteProfile() const;
    [[nodiscard]] RemoteServerProfile* selectedMutableRemoteProfile() noexcept;
    [[nodiscard]] RemoteTrack remoteTrackFromLibraryTrack(const RemoteServerProfile& profile, const TrackRecord& track) const;
    [[nodiscard]] bool startRemoteLibraryTrack(const TrackRecord& track);
    void refreshSearchResults();
    void applyEqualizerStateToPlayback();
    [[nodiscard]] ::lofibox::application::SourceProfileCommandService sourceProfileService() const noexcept;
    [[nodiscard]] ::lofibox::application::RemoteBrowseQueryService remoteBrowseService() const noexcept;
    void openRemoteProfile(std::size_t profile_index);
    void loadRemoteRoot();
    void openRemoteSetup();
    void openRemoteProfileSettings(std::size_t profile_index, int focus_row = 0);
    void openNewRemoteProfileSettings(RemoteServerKind kind);
    [[nodiscard]] bool persistRemoteProfiles();
    void beginRemoteProfileFieldEdit(int field);

    AppRuntimeState state_{};
    AppControllerSet controllers_{};
    RuntimeServices services_{};
};

} // namespace lofibox::app
