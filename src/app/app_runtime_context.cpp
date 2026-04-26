// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_runtime_context.h"

#include <utility>

#include "app/source_manager_projection.h"
#include "remote/common/remote_source_registry.h"

namespace lofibox::app {

namespace {
using clock = std::chrono::steady_clock;
}

AppRuntimeContext::AppRuntimeContext(std::vector<std::filesystem::path> media_roots,
                                     ui::UiAssets assets,
                                     RuntimeServices services)
    : state_{},
      services_(withNullRuntimeServices(std::move(services)))
{
    state_.media_roots = std::move(media_roots);
    state_.ui_assets = std::move(assets);
    state_.remote_profiles = services_.remote.remote_profile_store->loadProfiles();
    controllers_.bindServices(services_);
    refreshRuntimeStatus(true);
}

void AppRuntimeContext::update()
{
    updateAppLifecycle(*this);
}

void AppRuntimeContext::handleInput(const InputEvent& event)
{
    routeInput(*this, event);
}

void AppRuntimeContext::render(core::Canvas& canvas) const
{
    renderApp(canvas, *this);
}

AppDebugSnapshot AppRuntimeContext::snapshot() const
{
    const auto& playback = controllers_.playback.session();
    return AppDebugSnapshot{
        currentPage(),
        controllers_.library.state() == LibraryIndexState::Ready || controllers_.library.state() == LibraryIndexState::Degraded,
        controllers_.library.model().tracks.size(),
        pageModel().rows.size(),
        state_.navigation.list_selection.selected,
        state_.navigation.list_selection.scroll,
        playback.current_track_id.has_value(),
        playback.status,
        playback.current_track_id,
        playback.shuffle_enabled,
        playback.repeat_all,
        playback.repeat_one,
        state_.network.connected,
        static_cast<int>(state_.eq.bands.size()),
        state_.eq.selected_band};
}

bool AppRuntimeContext::bootAnimationComplete() const
{
    if (!state_.ui_assets.logo) {
        return true;
    }
    return (clock::now() - state_.boot_started) >= std::chrono::milliseconds{1450};
}

void AppRuntimeContext::refreshNetworkStatus()
{
    state_.network.connected = services_.connectivity.provider->connected();
    state_.network.status_message = state_.network.connected ? "ONLINE" : "OFFLINE";
}

void AppRuntimeContext::refreshMetadataServiceState()
{
    state_.metadata_service.available = services_.metadata.metadata_provider->available();
    state_.metadata_service.display_name = services_.metadata.metadata_provider->displayName();
    state_.metadata_service.status = !state_.metadata_service.available ? "UNAVAILABLE" : (state_.network.connected ? "ONLINE" : "OFFLINE");
}

void AppRuntimeContext::refreshRuntimeStatus(bool force)
{
    const auto now = clock::now();
    if (!force && state_.last_status_refresh != clock::time_point{} && now - state_.last_status_refresh < std::chrono::seconds{2}) {
        return;
    }
    state_.last_status_refresh = now;
    refreshNetworkStatus();
    refreshMetadataServiceState();
}

void AppRuntimeContext::refreshRuntimeStatusIfDue()
{
    refreshRuntimeStatus(false);
}

AppPage AppRuntimeContext::currentPage() const noexcept
{
    return state_.navigation.currentPage();
}

NavigationState& AppRuntimeContext::navigationState() noexcept
{
    return state_.navigation;
}

LibraryController& AppRuntimeContext::libraryController() noexcept
{
    return controllers_.library;
}

PlaybackController& AppRuntimeContext::playbackController() noexcept
{
    return controllers_.playback;
}

EqState& AppRuntimeContext::eqState() noexcept
{
    return state_.eq;
}

int& AppRuntimeContext::mainMenuIndex() noexcept
{
    return state_.main_menu_index;
}

void AppRuntimeContext::closeHelpForCommand() noexcept
{
    closeHelp();
}

std::chrono::steady_clock::time_point AppRuntimeContext::lastUpdate() const noexcept
{
    return state_.last_update;
}

void AppRuntimeContext::setLastUpdate(std::chrono::steady_clock::time_point value) noexcept
{
    state_.last_update = value;
}

const ui::UiAssets& AppRuntimeContext::assets() const noexcept
{
    return state_.ui_assets;
}

std::chrono::steady_clock::time_point AppRuntimeContext::bootStarted() const noexcept
{
    return state_.boot_started;
}

LibraryIndexState AppRuntimeContext::libraryState() const noexcept
{
    return controllers_.library.state();
}

void AppRuntimeContext::startLibraryLoading()
{
    controllers_.library.startLoading();
}

void AppRuntimeContext::refreshLibrary()
{
    controllers_.library.refreshLibrary(state_.media_roots, *services_.metadata.metadata_provider);
}

void AppRuntimeContext::showMainMenu()
{
    state_.navigation.replaceStack({AppPage::MainMenu});
}

void AppRuntimeContext::updatePlayback(double delta_seconds)
{
    controllers_.playback.update(delta_seconds, controllers_.library);
}

StorageInfo AppRuntimeContext::storage() const
{
    return controllers_.library.model().storage;
}

bool AppRuntimeContext::networkConnected() const noexcept
{
    return state_.network.connected;
}

int AppRuntimeContext::mainMenuIndex() const noexcept
{
    return state_.main_menu_index;
}

const PlaybackSession& AppRuntimeContext::playbackSession() const noexcept
{
    return controllers_.playback.session();
}

const EqState& AppRuntimeContext::eqState() const noexcept
{
    return state_.eq;
}

ListSelection AppRuntimeContext::listSelection() const noexcept
{
    return state_.navigation.list_selection;
}

AppPage AppRuntimeContext::helpPage() const noexcept
{
    return state_.help_page;
}

const TrackRecord* AppRuntimeContext::findTrack(int id) const noexcept
{
    return controllers_.library.findTrack(id);
}

void AppRuntimeContext::pushPage(AppPage page)
{
    commandPushPage(*this, page);
}

void AppRuntimeContext::popPage()
{
    commandPopPage(*this);
}

void AppRuntimeContext::closeHelp() noexcept
{
    state_.help_open = false;
}

void AppRuntimeContext::toggleHelpForCurrentPage() noexcept
{
    const auto page = currentPage();
    if (page == AppPage::Boot) {
        return;
    }
    if (state_.help_open && state_.help_page == page) {
        state_.help_open = false;
        return;
    }
    state_.help_page = page;
    state_.help_open = true;
}

bool AppRuntimeContext::helpOpen() const noexcept
{
    return state_.help_open;
}

void AppRuntimeContext::playFromMenu()
{
    commandPlayFromMenu(*this);
}

void AppRuntimeContext::pausePlayback()
{
    commandPausePlayback(*this);
}

void AppRuntimeContext::stepTrack(int delta)
{
    commandStepTrack(*this, delta);
}

void AppRuntimeContext::cycleMainMenuPlaybackMode()
{
    commandCycleMainMenuPlaybackMode(*this);
}

AppPageModel AppRuntimeContext::pageModel() const
{
    const auto page = currentPage();
    return buildAppPageModel(AppPageModelInput{
        page,
        controllers_.library.titleOverrideForPage(page),
        controllers_.library.rowsForPage(page),
        state_.settings,
        state_.network.connected,
        state_.metadata_service.display_name,
        buildSourceManagerRows(state_.remote_profiles, remote::RemoteSourceRegistry{}.manifests())});
}

void AppRuntimeContext::moveMainMenuSelection(int delta)
{
    commandMoveMainMenuSelection(*this, delta);
}

void AppRuntimeContext::resetMainMenuSelection() noexcept
{
    commandResetMainMenuSelection(*this);
}

void AppRuntimeContext::moveSelection(int delta)
{
    commandMoveSelection(*this, delta);
}

bool AppRuntimeContext::isBrowseListPage() const noexcept
{
    return lofibox::app::isBrowseListPage(currentPage());
}

bool AppRuntimeContext::nowPlayingConfirmBlocked() const
{
    return clock::now() < state_.now_playing_confirm_blocked_until;
}

void AppRuntimeContext::confirmMainMenu()
{
    commandConfirmMainMenu(*this);
}

void AppRuntimeContext::toggleShuffle()
{
    commandToggleShuffle(*this);
}

void AppRuntimeContext::cycleRepeatMode()
{
    commandCycleRepeatMode(*this);
}

void AppRuntimeContext::togglePlayPause()
{
    commandTogglePlayPause(*this);
}

void AppRuntimeContext::moveEqualizerSelection(int delta)
{
    commandMoveEqualizerSelection(*this, delta);
}

void AppRuntimeContext::adjustSelectedEqualizerBand(int delta)
{
    commandAdjustSelectedEqualizerBand(*this, delta);
}

void AppRuntimeContext::cycleSongSortModeAndClamp()
{
    commandCycleSongSortModeAndClamp(*this);
}

void AppRuntimeContext::confirmListPage()
{
    commandConfirmListPage(*this);
}

} // namespace lofibox::app
