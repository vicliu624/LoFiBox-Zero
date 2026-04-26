// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/lofibox_app.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "app/app_state.h"
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
constexpr int kMainMenuItemCount = 6;

struct LoFiBoxApp::Impl final : AppInputTarget, AppRenderTarget, AppLifecycleTarget {
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
        closeHelp();
        navigation.pushPage(page);
    }

    void popPage() override
    {
        closeHelp();
        navigation.popPage();
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

    void pausePlayback() override
    {
        playback_controller.pause();
    }

    void stepTrack(int delta) override
    {
        playback_controller.stepQueue(library_controller, delta);
    }

    void cycleMainMenuPlaybackMode() override
    {
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

    void clampListSelection()
    {
        const auto model = pageModel();
        navigation.clampListSelection(static_cast<int>(model.rows.size()), model.max_visible_rows);
    }

    void moveMainMenuSelection(int delta) override
    {
        main_menu_index = (main_menu_index + kMainMenuItemCount + delta) % kMainMenuItemCount;
    }

    void resetMainMenuSelection() noexcept override
    {
        main_menu_index = 0;
    }

    void moveSelection(int delta) override
    {
        const auto model = pageModel();
        navigation.moveSelection(delta, static_cast<int>(model.rows.size()), model.max_visible_rows);
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
        switch (main_menu_index) {
        case 0:
            library_controller.setSongsContextAll();
            pushPage(AppPage::Songs);
            break;
        case 1:
            pushPage(AppPage::MusicIndex);
            break;
        case 2:
            pushPage(AppPage::Playlists);
            break;
        case 3:
            pushPage(AppPage::NowPlaying);
            break;
        case 4:
            pushPage(AppPage::Equalizer);
            break;
        case 5:
            pushPage(AppPage::Settings);
            break;
        default:
            break;
        }
    }

    void toggleShuffle() override
    {
        playback_controller.toggleShuffle();
    }

    void cycleRepeatMode() override
    {
        playback_controller.cycleRepeatMode();
    }

    void togglePlayPause() override
    {
        playback_controller.togglePlayPause();
    }

    void moveEqualizerSelection(int delta) override
    {
        eq.selected_band = std::clamp(eq.selected_band + delta, 0, static_cast<int>(eq.bands.size()) - 1);
    }

    void adjustSelectedEqualizerBand(int delta) override
    {
        auto& band = eq.bands[static_cast<std::size_t>(eq.selected_band)];
        band = std::clamp(band + delta, -12, 12);
    }

    void cycleSongSortModeAndClamp() override
    {
        library_controller.cycleSongSortMode();
        clampListSelection();
    }

    void confirmListPage() override
    {
        const int selected = navigation.list_selection.selected;
        const auto page = currentPage();
        const auto library_result = library_controller.openSelectedListItem(page, selected);
        switch (library_result.kind) {
        case LibraryOpenResult::Kind::PushPage:
            pushPage(library_result.page);
            return;
        case LibraryOpenResult::Kind::StartTrack:
            if (playback_controller.startTrack(library_controller, library_result.track_id) && currentPage() != AppPage::NowPlaying) {
                pushPage(AppPage::NowPlaying);
            }
            return;
        case LibraryOpenResult::Kind::None:
            break;
        }

        switch (page) {
        case AppPage::Settings:
            if (selected == 6) {
                pushPage(AppPage::About);
            }
            break;
        default:
            break;
        }
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
