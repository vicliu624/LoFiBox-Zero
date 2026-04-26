// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_runtime_context.h"

#include <utility>

namespace lofibox::app {

namespace {
using clock = std::chrono::steady_clock;
}

AppRuntimeContext::AppRuntimeContext(std::vector<std::filesystem::path> media_roots,
                                     ui::UiAssets assets,
                                     RuntimeServices services)
    : media_roots_(std::move(media_roots)),
      ui_assets_(std::move(assets)),
      services_(withNullRuntimeServices(std::move(services)))
{
    playback_controller_.setServices(services_);
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
    const auto& playback = playback_controller_.session();
    return AppDebugSnapshot{
        currentPage(),
        library_controller_.state() == LibraryIndexState::Ready || library_controller_.state() == LibraryIndexState::Degraded,
        library_controller_.model().tracks.size(),
        pageModel().rows.size(),
        navigation_.list_selection.selected,
        navigation_.list_selection.scroll,
        playback.current_track_id.has_value(),
        playback.status,
        playback.current_track_id,
        playback.shuffle_enabled,
        playback.repeat_all,
        playback.repeat_one,
        network_.connected,
        static_cast<int>(eq_.bands.size()),
        eq_.selected_band};
}

bool AppRuntimeContext::bootAnimationComplete() const
{
    if (!ui_assets_.logo) {
        return true;
    }
    return (clock::now() - boot_started_) >= std::chrono::milliseconds{1450};
}

void AppRuntimeContext::refreshNetworkStatus()
{
    network_.connected = services_.connectivity_provider->connected();
    network_.status_message = network_.connected ? "ONLINE" : "OFFLINE";
}

void AppRuntimeContext::refreshMetadataServiceState()
{
    metadata_service_.available = services_.metadata_provider->available();
    metadata_service_.display_name = services_.metadata_provider->displayName();
    metadata_service_.status = !metadata_service_.available ? "UNAVAILABLE" : (network_.connected ? "ONLINE" : "OFFLINE");
}

void AppRuntimeContext::refreshRuntimeStatus(bool force)
{
    const auto now = clock::now();
    if (!force && last_status_refresh_ != clock::time_point{} && now - last_status_refresh_ < std::chrono::seconds{2}) {
        return;
    }
    last_status_refresh_ = now;
    refreshNetworkStatus();
    refreshMetadataServiceState();
}

void AppRuntimeContext::refreshRuntimeStatusIfDue()
{
    refreshRuntimeStatus(false);
}

AppPage AppRuntimeContext::currentPage() const noexcept
{
    return navigation_.currentPage();
}

NavigationState& AppRuntimeContext::navigationState() noexcept
{
    return navigation_;
}

LibraryController& AppRuntimeContext::libraryController() noexcept
{
    return library_controller_;
}

PlaybackController& AppRuntimeContext::playbackController() noexcept
{
    return playback_controller_;
}

EqState& AppRuntimeContext::eqState() noexcept
{
    return eq_;
}

int& AppRuntimeContext::mainMenuIndex() noexcept
{
    return main_menu_index_;
}

void AppRuntimeContext::closeHelpForCommand() noexcept
{
    closeHelp();
}

std::chrono::steady_clock::time_point AppRuntimeContext::lastUpdate() const noexcept
{
    return last_update_;
}

void AppRuntimeContext::setLastUpdate(std::chrono::steady_clock::time_point value) noexcept
{
    last_update_ = value;
}

const ui::UiAssets& AppRuntimeContext::assets() const noexcept
{
    return ui_assets_;
}

std::chrono::steady_clock::time_point AppRuntimeContext::bootStarted() const noexcept
{
    return boot_started_;
}

LibraryIndexState AppRuntimeContext::libraryState() const noexcept
{
    return library_controller_.state();
}

void AppRuntimeContext::startLibraryLoading()
{
    library_controller_.startLoading();
}

void AppRuntimeContext::refreshLibrary()
{
    library_controller_.refreshLibrary(media_roots_, *services_.metadata_provider);
}

void AppRuntimeContext::showMainMenu()
{
    navigation_.replaceStack({AppPage::MainMenu});
}

void AppRuntimeContext::updatePlayback(double delta_seconds)
{
    playback_controller_.update(delta_seconds, library_controller_);
}

StorageInfo AppRuntimeContext::storage() const
{
    return library_controller_.model().storage;
}

bool AppRuntimeContext::networkConnected() const noexcept
{
    return network_.connected;
}

int AppRuntimeContext::mainMenuIndex() const noexcept
{
    return main_menu_index_;
}

const PlaybackSession& AppRuntimeContext::playbackSession() const noexcept
{
    return playback_controller_.session();
}

const EqState& AppRuntimeContext::eqState() const noexcept
{
    return eq_;
}

ListSelection AppRuntimeContext::listSelection() const noexcept
{
    return navigation_.list_selection;
}

AppPage AppRuntimeContext::helpPage() const noexcept
{
    return help_page_;
}

const TrackRecord* AppRuntimeContext::findTrack(int id) const noexcept
{
    return library_controller_.findTrack(id);
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
    help_open_ = false;
}

void AppRuntimeContext::toggleHelpForCurrentPage() noexcept
{
    const auto page = currentPage();
    if (page == AppPage::Boot) {
        return;
    }
    if (help_open_ && help_page_ == page) {
        help_open_ = false;
        return;
    }
    help_page_ = page;
    help_open_ = true;
}

bool AppRuntimeContext::helpOpen() const noexcept
{
    return help_open_;
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
        library_controller_.titleOverrideForPage(page),
        library_controller_.rowsForPage(page),
        settings_,
        network_.connected,
        metadata_service_.display_name});
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
    return clock::now() < now_playing_confirm_blocked_until_;
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
