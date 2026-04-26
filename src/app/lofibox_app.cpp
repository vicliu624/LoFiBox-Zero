// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/lofibox_app.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/app_state.h"
#include "app/app_input_router.h"
#include "app/app_renderer.h"
#include "app/library_controller.h"
#include "app/navigation_state.h"
#include "app/playback_controller.h"
#include "ui/ui_primitives.h"
#include "ui/ui_theme.h"

namespace fs = std::filesystem;

namespace lofibox::app {
namespace {

using clock = std::chrono::steady_clock;
constexpr int kMainMenuItemCount = 6;

std::string_view pageTitleDefault(AppPage page) noexcept
{
    switch (page) {
    case AppPage::Boot: return "LOADING";
    case AppPage::MainMenu: return "LOFIBOX";
    case AppPage::MusicIndex: return "LIBRARY";
    case AppPage::Artists: return "ARTISTS";
    case AppPage::Albums: return "ALBUMS";
    case AppPage::Songs: return "SONGS";
    case AppPage::Genres: return "GENRES";
    case AppPage::Composers: return "COMPOSERS";
    case AppPage::Compilations: return "COMPILATIONS";
    case AppPage::Playlists: return "PLAYLISTS";
    case AppPage::PlaylistDetail: return "PLAYLIST";
    case AppPage::NowPlaying: return "NOW PLAYING";
    case AppPage::Lyrics: return "LYRICS";
    case AppPage::Equalizer: return "EQUALIZER";
    case AppPage::Settings: return "SETTINGS";
    case AppPage::About: return "ABOUT";
    }
    return "";
}

} // namespace

struct LoFiBoxApp::Impl final : AppInputTarget, AppRenderTarget {
    std::vector<fs::path> media_roots{};
    ui::UiAssets ui_assets{};
    LibraryController library_controller{};
    bool boot_ready{false};
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
    std::mt19937 random{12345U};

    [[nodiscard]] std::string networkStatusLabel() const
    {
        return network.connected ? "ONLINE" : "OFFLINE";
    }

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

    void refreshRuntimeStatusIfDue(bool force = false)
    {
        const auto now = clock::now();
        if (!force && last_status_refresh != clock::time_point{} && now - last_status_refresh < std::chrono::seconds{2}) {
            return;
        }
        last_status_refresh = now;
        refreshNetworkStatus();
        refreshMetadataServiceState();
    }

    [[nodiscard]] AppPage currentPage() const noexcept override
    {
        return navigation.currentPage();
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

    [[nodiscard]] TrackRecord* findMutableTrack(int id) noexcept
    {
        return library_controller.findMutableTrack(id);
    }

    void resetListSelection() noexcept
    {
        navigation.resetListSelection();
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

    [[nodiscard]] std::string pageTitle() const override
    {
        const auto page = currentPage();
        if (const auto override = library_controller.titleOverrideForPage(page)) {
            return ui::fitUpper(*override, 18);
        }
        return std::string(pageTitleDefault(page));
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>> currentRows() const override
    {
        std::vector<std::pair<std::string, std::string>> rows{};
        if (const auto library_rows = library_controller.rowsForPage(currentPage())) {
            return *library_rows;
        }
        switch (currentPage()) {
        case AppPage::Settings:
            return {{"NETWORK", networkStatusLabel()}, {"METADATA", metadata_service.display_name}, {"SLEEP TIMER", settings.sleep_timer_index == 0 ? "OFF" : "ON"}, {"BACKLIGHT", std::to_string(settings.backlight_index + 1)}, {"LANGUAGE", "EN"}, {"REMOTE MEDIA", "SERVERS"}, {"ABOUT", "INFO"}};
        default:
            return rows;
        }
    }

    void clampListSelection()
    {
        navigation.clampListSelection(static_cast<int>(currentRows().size()), ui::kMaxVisibleRows);
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
        navigation.moveSelection(delta, static_cast<int>(currentRows().size()), ui::kMaxVisibleRows);
    }

    [[nodiscard]] bool isBrowseListPage() const noexcept override
    {
        switch (currentPage()) {
        case AppPage::MusicIndex:
        case AppPage::Artists:
        case AppPage::Albums:
        case AppPage::Songs:
        case AppPage::Genres:
        case AppPage::Composers:
        case AppPage::Compilations:
        case AppPage::Playlists:
        case AppPage::PlaylistDetail:
            return true;
        default:
            return false;
        }
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
    impl_->refreshRuntimeStatusIfDue(true);
}

LoFiBoxApp::~LoFiBoxApp() = default;

void LoFiBoxApp::update()
{
    const auto now = clock::now();
    const double delta = std::chrono::duration<double>(now - impl_->last_update).count();
    impl_->last_update = now;
    impl_->refreshRuntimeStatusIfDue();

    if (impl_->library_controller.state() == LibraryIndexState::Uninitialized) {
        impl_->library_controller.startLoading();
        return;
    }

    if (impl_->library_controller.state() == LibraryIndexState::Loading) {
        impl_->library_controller.refreshLibrary(impl_->media_roots, *impl_->services.metadata_provider);
    }

    if (impl_->currentPage() == AppPage::Boot
        && impl_->library_controller.state() != LibraryIndexState::Uninitialized
        && impl_->library_controller.state() != LibraryIndexState::Loading
        && impl_->bootAnimationComplete()) {
        impl_->navigation.replaceStack({AppPage::MainMenu});
    }

    impl_->playback_controller.update(delta, impl_->library_controller);
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
        impl_->currentRows().size(),
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
