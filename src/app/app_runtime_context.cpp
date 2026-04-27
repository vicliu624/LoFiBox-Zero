// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_runtime_context.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "app/source_manager_projection.h"
#include "app/media_search_service.h"
#include "app/remote_profile_store.h"
#include "remote/common/remote_catalog_model.h"
#include "remote/common/remote_provider_contract.h"
#include "remote/common/remote_source_registry.h"
#include "remote/common/stream_source_model.h"

namespace lofibox::app {

namespace {
using clock = std::chrono::steady_clock;
constexpr int kLibraryLocalRowCount = 6;

RemoteServerKind kindForProtocol(StreamProtocol protocol) noexcept
{
    switch (protocol) {
    case StreamProtocol::Hls: return RemoteServerKind::Hls;
    case StreamProtocol::Dash: return RemoteServerKind::Dash;
    case StreamProtocol::Icecast:
    case StreamProtocol::Shoutcast:
        return RemoteServerKind::InternetRadio;
    case StreamProtocol::Smb: return RemoteServerKind::Smb;
    case StreamProtocol::Nfs: return RemoteServerKind::Nfs;
    case StreamProtocol::WebDav: return RemoteServerKind::WebDav;
    case StreamProtocol::Ftp: return RemoteServerKind::Ftp;
    case StreamProtocol::Sftp: return RemoteServerKind::Sftp;
    case StreamProtocol::DlnaUpnp: return RemoteServerKind::DlnaUpnp;
    case StreamProtocol::File:
    case StreamProtocol::Http:
    case StreamProtocol::Https:
        return RemoteServerKind::DirectUrl;
    }
    return RemoteServerKind::DirectUrl;
}
}

AppRuntimeContext::AppRuntimeContext(std::vector<std::filesystem::path> media_roots,
                                     ui::UiAssets assets,
                                     RuntimeServices services,
                                     std::vector<std::string> startup_uris)
    : state_{},
      services_(withNullRuntimeServices(std::move(services)))
{
    state_.media_roots = std::move(media_roots);
    state_.pending_open_uris = std::move(startup_uris);
    for (const auto& uri : state_.pending_open_uris) {
        std::error_code ec{};
        std::filesystem::path path{uri};
        if (!uri.empty() && uri.find("://") == std::string::npos && std::filesystem::exists(path, ec)) {
            state_.media_roots.push_back(path.parent_path());
        }
    }
    state_.ui_assets = std::move(assets);
    state_.remote_profiles = services_.remote.remote_profile_store->loadProfiles();
    controllers_.bindServices(services_);
    refreshRuntimeStatus(true);
}

void AppRuntimeContext::update()
{
    updateAppLifecycle(*this);
    handlePendingOpenRequests();
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

void AppRuntimeContext::handlePendingOpenRequests()
{
    if (state_.pending_open_processed || state_.pending_open_uris.empty()) {
        return;
    }
    const auto uri = state_.pending_open_uris.front();
    if (uri.empty()) {
        state_.pending_open_processed = true;
        return;
    }

    if (uri.find("://") != std::string::npos) {
        const auto entry = remote::StreamSourceClassifier::classify(uri);
        RemoteServerProfile profile{};
        profile.kind = kindForProtocol(entry.protocol);
        profile.id = "desktop-open";
        profile.name = "DESKTOP OPEN";
        profile.base_url = uri;
        state_.selected_remote_kind = profile.kind;
        state_.selected_remote_profile_index.reset();
        state_.selected_remote_session = services_.remote.remote_source_provider->probe(profile);
        RemoteTrack track{uri, "DESKTOP STREAM", "", "", "", 0};
        state_.selected_remote_node = RemoteCatalogNode{RemoteCatalogNodeKind::Tracks, uri, "DESKTOP STREAM", "", true, false};
        state_.selected_remote_stream = services_.remote.remote_stream_resolver->resolveTrack(profile, state_.selected_remote_session, track);
        if (state_.selected_remote_stream && controllers_.playback.startRemoteStream(*state_.selected_remote_stream, "DESKTOP STREAM", "DESKTOP")) {
            state_.navigation.replaceStack({AppPage::MainMenu, AppPage::NowPlaying});
        }
        state_.pending_open_processed = true;
        return;
    }

    if (controllers_.library.state() != LibraryIndexState::Ready && controllers_.library.state() != LibraryIndexState::Degraded) {
        return;
    }
    const std::filesystem::path requested{uri};
    std::error_code ec{};
    const auto absolute = std::filesystem::absolute(requested, ec);
    for (const auto& track : controllers_.library.model().tracks) {
        const auto track_absolute = std::filesystem::absolute(track.path, ec);
        if ((!ec && track_absolute == absolute) || track.path == requested) {
            if (controllers_.playback.startTrack(controllers_.library, track.id)) {
                state_.navigation.replaceStack({AppPage::MainMenu, AppPage::NowPlaying});
            }
            state_.pending_open_processed = true;
            return;
        }
    }
    state_.pending_open_processed = true;
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
    auto page_rows = currentPageRows();
    auto library_rows = controllers_.library.rowsForPage(page);
    if (page == AppPage::MusicIndex) {
        library_rows = libraryIndexRows();
    }
    return buildAppPageModel(AppPageModelInput{
        page,
        controllers_.library.titleOverrideForPage(page),
        library_rows,
        state_.settings,
        state_.network.connected,
        state_.metadata_service.display_name,
        buildSourceManagerRows(state_.remote_profiles, remote::RemoteSourceRegistry{}.manifests()),
        page_rows.empty() ? std::optional<std::vector<std::pair<std::string, std::string>>>{} : std::make_optional(std::move(page_rows))});
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

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::currentPageRows() const
{
    switch (currentPage()) {
    case AppPage::RemoteBrowse:
        return remoteBrowseRows();
    case AppPage::ServerDiagnostics:
        return serverDiagnosticsRows();
    case AppPage::StreamDetail:
        return streamDetailRows();
    case AppPage::Queue:
        return queueRows();
    case AppPage::Search:
        return searchRows();
    case AppPage::PlaylistEditor:
        return {{"CREATE PLAYLIST", "ENTER"}, {"ADD CURRENT", "F2"}, {"REMOVE ITEM", "DEL"}, {"REORDER", "UP/DOWN"}, {"SAVE", "F5"}};
    default:
        return {};
    }
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::libraryIndexRows() const
{
    auto rows = controllers_.library.rowsForPage(AppPage::MusicIndex).value_or(std::vector<std::pair<std::string, std::string>>{});
    for (const auto& profile : state_.remote_profiles) {
        const auto kind = remoteServerKindToString(profile.kind);
        const auto label = profile.name.empty() ? kind : profile.name;
        const bool has_endpoint = !profile.base_url.empty();
        const bool has_secret = !profile.password.empty() || !profile.api_token.empty();
        rows.emplace_back(label, has_endpoint && has_secret ? "REMOTE READY" : "REMOTE SETUP");
    }
    return rows;
}

int AppRuntimeContext::libraryRemoteRowBase() const
{
    return kLibraryLocalRowCount;
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::remoteBrowseRows() const
{
    std::vector<std::pair<std::string, std::string>> rows{};
    for (const auto& node : state_.remote_browse_nodes) {
        rows.emplace_back(node.title.empty() ? node.id : node.title, node.playable ? "PLAY" : (node.subtitle.empty() ? "BROWSE" : node.subtitle));
    }
    if (rows.empty()) {
        rows.emplace_back("NO REMOTE ITEMS", "EMPTY");
    }
    return rows;
}

std::optional<RemoteServerProfile> AppRuntimeContext::selectedRemoteProfile() const
{
    if (!state_.selected_remote_profile_index || *state_.selected_remote_profile_index >= state_.remote_profiles.size()) {
        return std::nullopt;
    }
    return state_.remote_profiles[*state_.selected_remote_profile_index];
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::serverDiagnosticsRows() const
{
    const auto profile = selectedRemoteProfile();
    std::string permission = "READ ONLY";
    if (state_.selected_remote_kind) {
        for (const auto& manifest : remote::RemoteSourceRegistry{}.manifests()) {
            if (manifest.kind == *state_.selected_remote_kind) {
                const auto writable = std::find(manifest.capabilities.begin(), manifest.capabilities.end(), remote::RemoteProviderCapability::WritableFavorites) != manifest.capabilities.end();
                permission = writable ? "READ/WRITE" : "READ ONLY";
                break;
            }
        }
    }
    return {
        {"SOURCE", profile ? (profile->name.empty() ? profile->base_url : profile->name) : "NOT SELECTED"},
        {"TYPE", state_.selected_remote_kind ? "CONFIGURED" : "UNKNOWN"},
        {"CONNECTION", state_.selected_remote_session.available ? "ONLINE" : "OFFLINE"},
        {"MESSAGE", state_.selected_remote_session.message.empty() ? "-" : state_.selected_remote_session.message},
        {"USER", profile && !profile->username.empty() ? profile->username : "-"},
        {"CREDENTIAL", profile && !profile->credential_ref.id.empty() ? "XDG STATE" : "NONE"},
        {"TLS", profile && !profile->tls_policy.verify_peer ? "EXCEPTION" : "VERIFY"},
        {"PERMISSION", permission},
        {"TOKEN", state_.selected_remote_session.access_token.empty() ? "NONE" : "REDACTED"},
    };
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::streamDetailRows() const
{
    if (!state_.selected_remote_stream) {
        return {{"STREAM", "NOT RESOLVED"}, {"ENTER", "RETRY"}};
    }
    const auto& diagnostics = state_.selected_remote_stream->diagnostics;
    return {
        {"SOURCE", diagnostics.source_name.empty() ? "REMOTE" : diagnostics.source_name},
        {"URL", diagnostics.resolved_url_redacted.empty() ? "REDACTED" : diagnostics.resolved_url_redacted},
        {"CONNECTION", diagnostics.connection_status.empty() ? (diagnostics.connected ? "READY" : "OFFLINE") : diagnostics.connection_status},
        {"BITRATE", diagnostics.bitrate_kbps > 0 ? std::to_string(diagnostics.bitrate_kbps) + "K" : "UNKNOWN"},
        {"CODEC", diagnostics.codec.empty() ? "UNKNOWN" : diagnostics.codec},
        {"LIVE", diagnostics.live ? "YES" : "NO"},
        {"ENTER", "PLAY"},
    };
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::queueRows() const
{
    const auto& playback = controllers_.playback.session();
    std::vector<std::pair<std::string, std::string>> rows{};
    if (playback.current_track_id) {
        if (const auto* track = controllers_.library.findTrack(*playback.current_track_id)) {
            rows.emplace_back(track->title, "LOCAL");
        }
    } else if (!playback.current_stream_title.empty()) {
        rows.emplace_back(playback.current_stream_title, playback.current_stream_live ? "LIVE" : "REMOTE");
    }
    if (rows.empty()) {
        rows.emplace_back("QUEUE EMPTY", "IDLE");
    }
    rows.emplace_back("MODE", playback.shuffle_enabled ? "SHUFFLE" : (playback.repeat_one ? "REPEAT ONE" : (playback.repeat_all ? "REPEAT ALL" : "ORDER")));
    return rows;
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::searchRows() const
{
    std::vector<std::pair<std::string, std::string>> rows{};
    rows.emplace_back("QUERY", state_.search_query.empty() ? "TYPE..." : state_.search_query);
    if (state_.search_query.empty()) {
        rows.emplace_back("LOCAL + REMOTE", "WAITING");
        return rows;
    }

    const auto local_result = MediaSearchService::searchLocal(controllers_.library.model(), state_.search_query, 4);
    for (const auto& item : local_result.local_items) {
        rows.emplace_back(item.title.empty() ? item.id : item.title, "LOCAL");
    }

    if (const auto profile = selectedRemoteProfile(); profile && state_.selected_remote_session.available) {
        const auto remote_tracks = services_.remote.remote_catalog_provider->searchTracks(*profile, state_.selected_remote_session, state_.search_query, 4);
        for (const auto& track : remote_tracks) {
            rows.emplace_back(track.title.empty() ? track.id : track.title, profile->name.empty() ? "REMOTE" : profile->name);
        }
    }

    if (rows.size() == 1) {
        rows.emplace_back("NO RESULTS", "EMPTY");
    }
    return rows;
}

void AppRuntimeContext::openRemoteProfile(std::size_t profile_index)
{
    if (profile_index >= state_.remote_profiles.size()) {
        return;
    }
    state_.selected_remote_profile_index = profile_index;
    state_.selected_remote_kind = state_.remote_profiles[profile_index].kind;
    state_.selected_remote_session = services_.remote.remote_source_provider->probe(state_.remote_profiles[profile_index]);
    loadRemoteRoot();
}

void AppRuntimeContext::loadRemoteRoot()
{
    state_.selected_remote_parent = RemoteCatalogNode{};
    const auto profile = selectedRemoteProfile();
    if (!profile || !state_.selected_remote_session.available) {
        state_.remote_browse_nodes = remote::RemoteCatalogMap::rootNodes();
        return;
    }
    state_.remote_browse_nodes = services_.remote.remote_catalog_provider->browse(*profile, state_.selected_remote_session, state_.selected_remote_parent, 100);
    if (state_.remote_browse_nodes.empty()) {
        state_.remote_browse_nodes = remote::RemoteCatalogMap::rootNodes();
    }
}

bool AppRuntimeContext::handleLibraryRemoteConfirm(int selected)
{
    const int remote_base = libraryRemoteRowBase();
    if (selected < remote_base) {
        return false;
    }
    const auto profile_index = static_cast<std::size_t>(selected - remote_base);
    if (profile_index >= state_.remote_profiles.size()) {
        return false;
    }
    openRemoteProfile(profile_index);
    commandPushPage(*this, state_.selected_remote_session.available ? AppPage::RemoteBrowse : AppPage::ServerDiagnostics);
    return true;
}

bool AppRuntimeContext::handleSourceManagerConfirm(int selected)
{
    const auto manifests = remote::RemoteSourceRegistry{}.manifests();
    if (selected == 0) {
        commandPushPage(*this, AppPage::MusicIndex);
        return true;
    }

    const int profile_base = 3 + static_cast<int>(manifests.size());
    if (selected >= profile_base) {
        const auto profile_index = static_cast<std::size_t>(selected - profile_base);
        openRemoteProfile(profile_index);
        commandPushPage(*this, AppPage::RemoteBrowse);
        return true;
    }

    if (selected == 1 || selected == 2) {
        const RemoteServerKind wanted = selected == 1 ? RemoteServerKind::DirectUrl : RemoteServerKind::InternetRadio;
        const auto it = std::find_if(state_.remote_profiles.begin(), state_.remote_profiles.end(), [wanted](const RemoteServerProfile& profile) {
            return profile.kind == wanted;
        });
        if (it != state_.remote_profiles.end()) {
            openRemoteProfile(static_cast<std::size_t>(std::distance(state_.remote_profiles.begin(), it)));
            commandPushPage(*this, AppPage::RemoteBrowse);
        } else {
            state_.selected_remote_kind = wanted;
            state_.selected_remote_profile_index.reset();
            state_.selected_remote_session = {};
            state_.remote_browse_nodes = {};
            commandPushPage(*this, AppPage::ServerDiagnostics);
        }
        return true;
    }

    const int manifest_index = selected - 3;
    if (manifest_index >= 0 && manifest_index < static_cast<int>(manifests.size())) {
        state_.selected_remote_kind = manifests[static_cast<std::size_t>(manifest_index)].kind;
        state_.selected_remote_profile_index.reset();
        state_.selected_remote_session = {};
        state_.remote_browse_nodes = {};
        commandPushPage(*this, AppPage::ServerDiagnostics);
        return true;
    }
    return false;
}

bool AppRuntimeContext::handleRemoteBrowseConfirm(int selected)
{
    if (selected < 0 || selected >= static_cast<int>(state_.remote_browse_nodes.size())) {
        return false;
    }
    const auto profile = selectedRemoteProfile();
    if (!profile) {
        return false;
    }

    const auto node = state_.remote_browse_nodes[static_cast<std::size_t>(selected)];
    state_.selected_remote_node = node;
    if (node.playable) {
        RemoteTrack track{node.id, node.title, node.subtitle, "", "", 0};
        state_.selected_remote_stream = services_.remote.remote_stream_resolver->resolveTrack(*profile, state_.selected_remote_session, track);
        commandPushPage(*this, AppPage::StreamDetail);
        return true;
    }

    if (node.browsable) {
        state_.selected_remote_parent = node;
        state_.remote_browse_nodes = services_.remote.remote_catalog_provider->browse(*profile, state_.selected_remote_session, node, 100);
        if (state_.remote_browse_nodes.empty()) {
            state_.remote_browse_nodes.push_back(RemoteCatalogNode{RemoteCatalogNodeKind::Root, "empty", "NO ITEMS", remote::RemoteCatalogMap::explainNode(node.kind), false, false});
        }
        state_.navigation.list_selection.selected = 0;
        state_.navigation.list_selection.scroll = 0;
        return true;
    }
    return false;
}

bool AppRuntimeContext::handleStreamDetailConfirm()
{
    if (!state_.selected_remote_stream) {
        const auto profile = selectedRemoteProfile();
        if (!profile || !state_.selected_remote_node) {
            return false;
        }
        RemoteTrack track{state_.selected_remote_node->id, state_.selected_remote_node->title, state_.selected_remote_node->subtitle, "", "", 0};
        state_.selected_remote_stream = services_.remote.remote_stream_resolver->resolveTrack(*profile, state_.selected_remote_session, track);
    }
    if (!state_.selected_remote_stream) {
        return false;
    }
    const auto title = state_.selected_remote_node ? state_.selected_remote_node->title : std::string{"REMOTE STREAM"};
    const auto profile = selectedRemoteProfile();
    const auto source = profile && !profile->name.empty() ? profile->name : std::string{"REMOTE"};
    return controllers_.playback.startRemoteStream(*state_.selected_remote_stream, title, source);
}

bool AppRuntimeContext::handleSearchConfirm(int selected)
{
    if (selected <= 0 || state_.search_query.empty()) {
        return false;
    }
    int cursor = 1;
    const auto local_result = MediaSearchService::searchLocal(controllers_.library.model(), state_.search_query, 4);
    for (const auto& item : local_result.local_items) {
        if (cursor == selected) {
            return item.local_track_id ? controllers_.playback.startTrack(controllers_.library, *item.local_track_id) : false;
        }
        ++cursor;
    }

    const auto profile = selectedRemoteProfile();
    if (!profile || !state_.selected_remote_session.available) {
        return false;
    }
    const auto remote_tracks = services_.remote.remote_catalog_provider->searchTracks(*profile, state_.selected_remote_session, state_.search_query, 4);
    for (const auto& track : remote_tracks) {
        if (cursor == selected) {
            state_.selected_remote_node = RemoteCatalogNode{RemoteCatalogNodeKind::Tracks, track.id, track.title, track.artist, true, false};
            state_.selected_remote_stream = services_.remote.remote_stream_resolver->resolveTrack(*profile, state_.selected_remote_session, track);
            return state_.selected_remote_stream
                ? controllers_.playback.startRemoteStream(*state_.selected_remote_stream, track.title, profile->name.empty() ? std::string{"REMOTE"} : profile->name)
                : false;
        }
        ++cursor;
    }
    return false;
}

void AppRuntimeContext::appendSearchCharacter(char ch)
{
    if (state_.search_query.size() < 64) {
        state_.search_query.push_back(ch);
    }
}

void AppRuntimeContext::backspaceSearchQuery()
{
    if (!state_.search_query.empty()) {
        state_.search_query.pop_back();
    }
}

} // namespace lofibox::app
