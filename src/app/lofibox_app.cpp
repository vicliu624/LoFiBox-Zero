// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/lofibox_app.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "app/app_state.h"
#include "app/app_command_executor.h"
#include "app/app_input_router.h"
#include "app/app_lifecycle.h"
#include "app/app_page_model.h"
#include "app/app_renderer.h"
#include "app/library_controller.h"
#include "app/navigation_state.h"
#include "app/playback_controller.h"

namespace fs = std::filesystem;

namespace lofibox::app {
using clock = std::chrono::steady_clock;

struct LoFiBoxApp::Impl final : AppInputTarget, AppRenderTarget, AppLifecycleTarget, AppCommandTarget {
    std::vector<fs::path> media_roots{};
    ui::UiAssets ui_assets{};
    LibraryController library_controller{};
    clock::time_point boot_started{clock::now()};
    NavigationState navigation{};
    int main_menu_index{1};
    SettingsState settings{};
    NetworkState network{};
    MetadataServiceState metadata_service{};
    EqState eq{};
    PlaybackController playback_controller{};
    RuntimeServices services{};
    clock::time_point last_update{clock::now()};
    clock::time_point last_status_refresh{};
    clock::time_point now_playing_confirm_blocked_until{};
    bool help_open{false};
    AppPage help_page{AppPage::MainMenu};

    [[nodiscard]] bool bootAnimationComplete() const
    {
        if (!ui_assets.logo) {
            return true;
        }
        return (clock::now() - boot_started) >= std::chrono::milliseconds{1450};
    }

    void refreshNetworkStatus()
    {
        network.connected = services.connectivity_provider->connected();
        network.status_message = network.connected ? "ONLINE" : "OFFLINE";
    }

    void refreshMetadataServiceState()
    {
        metadata_service.available = services.metadata_provider->available();
        metadata_service.display_name = services.metadata_provider->displayName();
        metadata_service.status = !metadata_service.available ? "UNAVAILABLE" : (network.connected ? "ONLINE" : "OFFLINE");
    }

    void refreshRuntimeStatus(bool force)
    {
        const auto now = clock::now();
        if (!force && last_status_refresh != clock::time_point{} && now - last_status_refresh < std::chrono::seconds{2}) {
            return;
        }
        last_status_refresh = now;
        refreshNetworkStatus();
        refreshMetadataServiceState();
    }

    void refreshRuntimeStatusIfDue() override
    {
        refreshRuntimeStatus(false);
    }

    [[nodiscard]] AppPage currentPage() const noexcept override
    {
        return navigation.currentPage();
    }

    NavigationState& navigationState() noexcept override
    {
        return navigation;
    }

    LibraryController& libraryController() noexcept override
    {
        return library_controller;
    }

    PlaybackController& playbackController() noexcept override
    {
        return playback_controller;
    }

    EqState& eqState() noexcept override
    {
        return eq;
    }

    int& mainMenuIndex() noexcept override
    {
        return main_menu_index;
    }

    void closeHelpForCommand() noexcept override
    {
        closeHelp();
    }

    [[nodiscard]] clock::time_point lastUpdate() const noexcept override
    {
        return last_update;
    }

    void setLastUpdate(clock::time_point value) noexcept override
    {
        last_update = value;
    }

    [[nodiscard]] const ui::UiAssets& assets() const noexcept override
    {
        return ui_assets;
    }

    [[nodiscard]] clock::time_point bootStarted() const noexcept override
    {
        return boot_started;
    }

    [[nodiscard]] LibraryIndexState libraryState() const noexcept override
    {
        return library_controller.state();
    }

    void startLibraryLoading() override
    {
        library_controller.startLoading();
    }

    void refreshLibrary() override
    {
        library_controller.refreshLibrary(media_roots, *services.metadata_provider);
    }

    void showMainMenu() override
    {
        navigation.replaceStack({AppPage::MainMenu});
    }

    void updatePlayback(double delta_seconds) override
    {
        playback_controller.update(delta_seconds, library_controller);
    }

    [[nodiscard]] StorageInfo storage() const override
    {
        return library_controller.model().storage;
    }

    [[nodiscard]] bool networkConnected() const noexcept override
    {
        return network.connected;
    }

    [[nodiscard]] int mainMenuIndex() const noexcept override
    {
        return main_menu_index;
    }

    [[nodiscard]] const PlaybackSession& playbackSession() const noexcept override
    {
        return playback_controller.session();
    }

    [[nodiscard]] const EqState& eqState() const noexcept override
    {
        return eq;
    }

    [[nodiscard]] ListSelection listSelection() const noexcept override
    {
        return navigation.list_selection;
    }

    [[nodiscard]] AppPage helpPage() const noexcept override
    {
        return help_page;
    }

    [[nodiscard]] const TrackRecord* findTrack(int id) const noexcept override
    {
        return library_controller.findTrack(id);
    }

    void pushPage(AppPage page) override
    {
        commandPushPage(*this, page);
    }

    void popPage() override
    {
        commandPopPage(*this);
    }

    void closeHelp() noexcept override
    {
        help_open = false;
    }

    void toggleHelpForCurrentPage() noexcept override
    {
        const auto page = currentPage();
        if (page == AppPage::Boot) {
            return;
        }
        if (help_open && help_page == page) {
            help_open = false;
            return;
        }
        help_page = page;
        help_open = true;
    }

    [[nodiscard]] bool helpOpen() const noexcept override
    {
        return help_open;
    }

    void playFromMenu() override
    {
        commandPlayFromMenu(*this);
    }

    void pausePlayback() override
    {
        commandPausePlayback(*this);
    }

    void stepTrack(int delta) override
    {
        commandStepTrack(*this, delta);
    }

    void cycleMainMenuPlaybackMode() override
    {
        commandCycleMainMenuPlaybackMode(*this);
    }

    [[nodiscard]] AppPageModel pageModel() const override
    {
        const auto page = currentPage();
        return buildAppPageModel(AppPageModelInput{
            page,
            library_controller.titleOverrideForPage(page),
            library_controller.rowsForPage(page),
            settings,
            network.connected,
            metadata_service.display_name});
    }

    void moveMainMenuSelection(int delta) override
    {
        commandMoveMainMenuSelection(*this, delta);
    }

    void resetMainMenuSelection() noexcept override
    {
        commandResetMainMenuSelection(*this);
    }

    void moveSelection(int delta) override
    {
        commandMoveSelection(*this, delta);
    }

    [[nodiscard]] bool isBrowseListPage() const noexcept override
    {
        return lofibox::app::isBrowseListPage(currentPage());
    }

    bool nowPlayingConfirmBlocked() const override
    {
        return clock::now() < now_playing_confirm_blocked_until;
    }

    void confirmMainMenu() override
    {
        commandConfirmMainMenu(*this);
    }

    void toggleShuffle() override
    {
        commandToggleShuffle(*this);
    }

    void cycleRepeatMode() override
    {
        commandCycleRepeatMode(*this);
    }

    void togglePlayPause() override
    {
        commandTogglePlayPause(*this);
    }

    void moveEqualizerSelection(int delta) override
    {
        commandMoveEqualizerSelection(*this, delta);
    }

    void adjustSelectedEqualizerBand(int delta) override
    {
        commandAdjustSelectedEqualizerBand(*this, delta);
    }

    void cycleSongSortModeAndClamp() override
    {
        commandCycleSongSortModeAndClamp(*this);
    }

    void confirmListPage() override
    {
        commandConfirmListPage(*this);
    }
};

LoFiBoxApp::LoFiBoxApp(std::vector<std::filesystem::path> media_roots, ui::UiAssets assets, RuntimeServices services)
    : impl_(std::make_unique<Impl>())
{
    impl_->media_roots = std::move(media_roots);
    impl_->ui_assets = std::move(assets);
    impl_->services = withNullRuntimeServices(std::move(services));
    impl_->playback_controller.setServices(impl_->services);
    impl_->refreshRuntimeStatus(true);
}

LoFiBoxApp::~LoFiBoxApp() = default;

void LoFiBoxApp::update()
{
    updateAppLifecycle(*impl_);
}

void LoFiBoxApp::handleInput(const InputEvent& event)
{
    routeInput(*impl_, event);
}

void LoFiBoxApp::render(core::Canvas& canvas) const
{
    renderApp(canvas, *impl_);
}

AppDebugSnapshot LoFiBoxApp::snapshot() const
{
    const auto& playback = impl_->playback_controller.session();
    return AppDebugSnapshot{
        impl_->currentPage(),
        impl_->library_controller.state() == LibraryIndexState::Ready || impl_->library_controller.state() == LibraryIndexState::Degraded,
        impl_->library_controller.model().tracks.size(),
        impl_->pageModel().rows.size(),
        impl_->navigation.list_selection.selected,
        impl_->navigation.list_selection.scroll,
        playback.current_track_id.has_value(),
        playback.status,
        playback.current_track_id,
        playback.shuffle_enabled,
        playback.repeat_all,
        playback.repeat_one,
        impl_->network.connected,
        static_cast<int>(impl_->eq.bands.size()),
        impl_->eq.selected_band};
}

} // namespace lofibox::app
