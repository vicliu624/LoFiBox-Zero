// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_runtime_context.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "app/media_search_service.h"
#include "app/remote_media_contract.h"
#include "app/remote_profile_store.h"
#include "app/source_manager_projection.h"
#include "cache/cache_manager.h"
#include "remote/common/remote_catalog_model.h"
#include "remote/common/remote_provider_contract.h"
#include "remote/common/remote_source_registry.h"
#include "remote/common/stream_source_model.h"
#include "playback/streaming_playback_policy.h"

namespace lofibox::app {

namespace {
using clock = std::chrono::steady_clock;

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

std::string qualityLabel(StreamQualityPreference preference)
{
    switch (preference) {
    case StreamQualityPreference::Original: return "ORIGINAL";
    case StreamQualityPreference::Auto: return "AUTO";
    case StreamQualityPreference::LowBandwidth: return "128K";
    case StreamQualityPreference::Manual: return "MANUAL";
    }
    return "AUTO";
}

std::string bufferDecisionLabel(BufferDecision decision)
{
    switch (decision) {
    case BufferDecision::StartImmediately: return "READY";
    case BufferDecision::WaitForMinimumBuffer: return "BUFFERING";
    case BufferDecision::PauseAndRebuffer: return "REBUFFER";
    }
    return "UNKNOWN";
}

constexpr int kRemoteProfileRowProfile = 0;
constexpr int kRemoteProfileRowKind = 1;
constexpr int kRemoteProfileRowName = 2;
constexpr int kRemoteProfileRowAddress = 3;
constexpr int kRemoteProfileRowUsername = 4;
constexpr int kRemoteProfileRowPassword = 5;
constexpr int kRemoteProfileRowToken = 6;
constexpr int kRemoteProfileRowTlsVerify = 7;
constexpr int kRemoteProfileRowSelfSigned = 8;
constexpr int kRemoteProfileRowPermission = 9;
constexpr int kRemoteProfileRowTest = 10;
constexpr int kRemoteProfileRowSave = 11;
constexpr int kUnifiedRemoteLibraryTrackLimit = 10000;

enum class RemoteProfileCredentialMode {
    None,
    Optional,
    Required,
};

std::string remoteKindDisplayName(RemoteServerKind kind)
{
    const auto manifest = remote::remoteProviderManifest(kind);
    return manifest.display_name.empty() ? remoteServerKindToString(kind) : manifest.display_name;
}

std::string defaultRemoteProfileId(RemoteServerKind kind, std::size_t index)
{
    return remoteServerKindToString(kind) + "-" + std::to_string(static_cast<int>(index + 1U));
}

std::string defaultRemoteProfileName(RemoteServerKind kind)
{
    auto name = remoteKindDisplayName(kind);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return name;
}

std::string remoteProfileLabel(const RemoteServerProfile& profile)
{
    if (!profile.name.empty()) {
        return profile.name;
    }
    if (!profile.base_url.empty()) {
        return profile.base_url;
    }
    return defaultRemoteProfileName(profile.kind);
}

RemoteProfileCredentialMode remoteProfileCredentialMode(RemoteServerKind kind) noexcept
{
    switch (kind) {
    case RemoteServerKind::Jellyfin:
    case RemoteServerKind::OpenSubsonic:
    case RemoteServerKind::Navidrome:
    case RemoteServerKind::Emby:
        return RemoteProfileCredentialMode::Required;
    case RemoteServerKind::PlaylistManifest:
    case RemoteServerKind::WebDav:
    case RemoteServerKind::Ftp:
    case RemoteServerKind::Sftp:
        return RemoteProfileCredentialMode::Optional;
    case RemoteServerKind::DirectUrl:
    case RemoteServerKind::InternetRadio:
    case RemoteServerKind::Hls:
    case RemoteServerKind::Dash:
    case RemoteServerKind::Smb:
    case RemoteServerKind::Nfs:
    case RemoteServerKind::DlnaUpnp:
        return RemoteProfileCredentialMode::None;
    }
    return RemoteProfileCredentialMode::None;
}

bool remoteProfileSupportsCredentials(RemoteServerKind kind) noexcept
{
    return remoteProfileCredentialMode(kind) != RemoteProfileCredentialMode::None;
}

std::string remoteProfileReadiness(const RemoteServerProfile& profile)
{
    if (profile.base_url.empty()) {
        return "NEEDS URL";
    }
    if (remoteProfileCredentialMode(profile.kind) != RemoteProfileCredentialMode::Required) {
        return "READY";
    }
    if (profile.username.empty()) {
        return "NEEDS USER";
    }
    if (profile.password.empty()
        && profile.api_token.empty()
        && profile.credential_ref.id.empty()) {
        return "NEEDS SECRET";
    }
    return "READY";
}

std::string remoteCredentialValueLabel(RemoteServerKind kind, std::string_view value)
{
    const auto mode = remoteProfileCredentialMode(kind);
    if (mode == RemoteProfileCredentialMode::None) {
        return "N/A";
    }
    if (value.empty() && mode == RemoteProfileCredentialMode::Optional) {
        return "OPTIONAL";
    }
    return value.empty() ? "EMPTY" : "SET";
}

std::string remoteUsernameLabel(const RemoteServerProfile& profile)
{
    const auto mode = remoteProfileCredentialMode(profile.kind);
    if (mode == RemoteProfileCredentialMode::None) {
        return "N/A";
    }
    if (profile.username.empty()) {
        return mode == RemoteProfileCredentialMode::Optional ? "OPTIONAL" : "MISSING";
    }
    return profile.username;
}

std::string permissionLabel(RemoteServerKind kind)
{
    const auto manifest = remote::remoteProviderManifest(kind);
    if (remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::WritableMetadata)
        || remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::WritableFavorites)) {
        return "READ/WRITE";
    }
    return "READ ONLY";
}

bool remoteProfileKeepsLocalFacts(RemoteServerKind kind)
{
    const auto manifest = remote::remoteProviderManifest(kind);
    const bool writable_metadata =
        remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::WritableMetadata)
        && !remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::ReadOnly);
    return !writable_metadata;
}

std::string remoteEditFieldName(int field)
{
    switch (field) {
    case kRemoteProfileRowName: return "LABEL";
    case kRemoteProfileRowAddress: return "ADDRESS";
    case kRemoteProfileRowUsername: return "USER";
    case kRemoteProfileRowPassword: return "PASSWORD";
    case kRemoteProfileRowToken: return "API TOKEN";
    }
    return "FIELD";
}

std::string hiddenSecretLabel(std::string_view value)
{
    return value.empty() ? "EMPTY" : "SET";
}

RemoteTrack preferCachedRemoteTrackFacts(RemoteTrack current, const RemoteTrack& cached)
{
    if (!cached.title.empty()) current.title = cached.title;
    if (!cached.artist.empty()) current.artist = cached.artist;
    if (!cached.album.empty()) current.album = cached.album;
    if (!cached.album_id.empty()) current.album_id = cached.album_id;
    if (cached.duration_seconds > 0) current.duration_seconds = cached.duration_seconds;
    if (!cached.source_id.empty()) current.source_id = cached.source_id;
    if (!cached.source_label.empty()) current.source_label = cached.source_label;
    if (!cached.artwork_key.empty()) current.artwork_key = cached.artwork_key;
    if (!cached.artwork_url.empty()) current.artwork_url = cached.artwork_url;
    if (!cached.lyrics_plain.empty()) current.lyrics_plain = cached.lyrics_plain;
    if (!cached.lyrics_synced.empty()) current.lyrics_synced = cached.lyrics_synced;
    if (!cached.lyrics_source.empty()) current.lyrics_source = cached.lyrics_source;
    if (!cached.fingerprint.empty()) current.fingerprint = cached.fingerprint;
    return current;
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
        track.source_id = profile.id;
        track.source_label = profile.name;
        state_.selected_remote_node = RemoteCatalogNode{
            RemoteCatalogNodeKind::Tracks,
            uri,
            "DESKTOP STREAM",
            "",
            true,
            false};
        rememberRemoteTrackFacts(profile, track);
        state_.selected_remote_stream = services_.remote.remote_stream_resolver->resolveTrack(profile, state_.selected_remote_session, track);
        if (state_.selected_remote_stream && controllers_.playback.startRemoteStream(*state_.selected_remote_stream, track, "DESKTOP")) {
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
    refreshRemoteLibraryTracks();
}

void AppRuntimeContext::refreshRemoteLibraryTracks()
{
    if (!services_.remote.remote_source_provider->available() || !services_.remote.remote_catalog_provider->available()) {
        return;
    }

    for (auto& profile : state_.remote_profiles) {
        if (profile.base_url.empty()) {
            continue;
        }
        if (remoteProfileReadiness(profile) != "READY") {
            continue;
        }

        ensureSelectedProfileCredentialRef(profile);
        const auto session = services_.remote.remote_source_provider->probe(profile);
        if (!session.available) {
            continue;
        }

        auto tracks = services_.remote.remote_catalog_provider->libraryTracks(profile, session, kUnifiedRemoteLibraryTrackLimit);
        for (auto& track : tracks) {
            if (track.source_id.empty()) {
                track.source_id = profile.id;
            }
            if (track.source_label.empty()) {
                track.source_label = remoteProfileLabel(profile);
            }
            if (services_.cache.cache_manager && !track.id.empty()) {
                const auto cached = services_.cache.cache_manager->getText(
                    ::lofibox::cache::CacheBucket::Metadata,
                    remoteMediaCacheKey(profile, track.id));
                if (cached) {
                    track = preferCachedRemoteTrackFacts(std::move(track), parseRemoteTrackCache(*cached));
                }
            }
            rememberRemoteTrackFacts(profile, track);
        }

        controllers_.library.mergeRemoteTracks(profile, tracks);
    }
}

void AppRuntimeContext::showMainMenu()
{
    state_.navigation.replaceStack({AppPage::MainMenu});
}

void AppRuntimeContext::updatePlayback(double delta_seconds)
{
    controllers_.playback.update(delta_seconds, controllers_.library, [this](int track_id) {
        const auto* track = controllers_.library.findTrack(track_id);
        return track != nullptr && startRemoteLibraryTrack(*track);
    });
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
    controllers_.playback.stepQueue(controllers_.library, delta, [this](int track_id) {
        const auto* track = controllers_.library.findTrack(track_id);
        return track != nullptr && startRemoteLibraryTrack(*track);
    });
}

void AppRuntimeContext::cycleMainMenuPlaybackMode()
{
    commandCycleMainMenuPlaybackMode(*this);
}

void AppRuntimeContext::toggleRepeatAll()
{
    commandToggleRepeatAll(*this);
}

void AppRuntimeContext::toggleRepeatOne()
{
    commandToggleRepeatOne(*this);
}

void AppRuntimeContext::openSearchPage()
{
    commandPushPage(*this, AppPage::Search);
}

void AppRuntimeContext::openLibraryPage()
{
    commandPushPage(*this, AppPage::MusicIndex);
}

void AppRuntimeContext::openQueuePage()
{
    commandPushPage(*this, AppPage::Queue);
}

void AppRuntimeContext::openSettingsPage()
{
    commandPushPage(*this, AppPage::Settings);
}

void AppRuntimeContext::showMainMenuPage()
{
    closeHelp();
    showMainMenu();
}

AppPageModel AppRuntimeContext::pageModel() const
{
    const auto page = currentPage();
    auto page_rows = currentPageRows();
    auto library_rows = controllers_.library.rowsForPage(page);
    if (page == AppPage::MusicIndex) {
        library_rows = libraryIndexRows();
    }
    auto title_override = controllers_.library.titleOverrideForPage(page);
    if (page == AppPage::RemoteBrowse) {
        title_override = remoteBrowseTitleOverride();
    } else if (page == AppPage::RemoteProfileSettings) {
        if (const auto profile = selectedRemoteProfile()) {
            title_override = remoteKindDisplayName(profile->kind);
        }
    } else if (page == AppPage::ServerDiagnostics || page == AppPage::StreamDetail) {
        if (const auto profile = selectedRemoteProfile()) {
            title_override = remoteProfileLabel(*profile);
        }
    }
    return buildAppPageModel(AppPageModelInput{
        page,
        title_override,
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

void AppRuntimeContext::moveSelectionPage(int delta_pages)
{
    commandMoveSelectionPage(*this, delta_pages);
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

bool AppRuntimeContext::startLibraryTrack(int track_id)
{
    const auto* track = controllers_.library.findTrack(track_id);
    if (track == nullptr) {
        return false;
    }
    if (!track->remote) {
        return controllers_.playback.startTrack(controllers_.library, track_id);
    }
    controllers_.playback.prepareQueueForTrack(controllers_.library, track_id);
    return startRemoteLibraryTrack(*track);
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
    case AppPage::RemoteSetup:
        return remoteSetupRows();
    case AppPage::RemoteProfileSettings:
        return remoteProfileSettingsRows();
    case AppPage::RemoteFieldEditor:
        return remoteFieldEditorRows();
    case AppPage::PlaylistEditor:
        return {{"CREATE PLAYLIST", "OK"}, {"ADD CURRENT", "INS"}, {"REMOVE ITEM", "DEL"}, {"REORDER", "UP/DOWN"}, {"SAVE", "OK"}};
    default:
        return {};
    }
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::libraryIndexRows() const
{
    auto rows = controllers_.library.rowsForPage(AppPage::MusicIndex).value_or(std::vector<std::pair<std::string, std::string>>{});
    for (const auto& profile : state_.remote_profiles) {
        int track_count = 0;
        for (const auto& track : controllers_.library.model().tracks) {
            if (track.remote && track.remote_profile_id == profile.id) {
                ++track_count;
            }
        }
        rows.emplace_back(remoteProfileLabel(profile), std::to_string(track_count));
    }
    return rows;
}

int AppRuntimeContext::libraryRemoteRowBase() const
{
    const auto rows = controllers_.library.rowsForPage(AppPage::MusicIndex).value_or(std::vector<std::pair<std::string, std::string>>{});
    return static_cast<int>(rows.size());
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::remoteBrowseRows() const
{
    const auto browse_secondary = [](const RemoteCatalogNode& node) {
        if (node.playable) {
            if (!node.artist.empty() && !node.album.empty()) {
                return node.artist + " - " + node.album;
            }
            if (!node.artist.empty()) {
                return node.artist;
            }
            if (!node.album.empty()) {
                return node.album;
            }
            if (!node.subtitle.empty()) {
                return node.subtitle;
            }
            return std::string{"TRACK"};
        }
        return node.subtitle;
    };

    std::vector<std::pair<std::string, std::string>> rows{};
    for (const auto& node : state_.remote_browse_nodes) {
        rows.emplace_back(node.title.empty() ? node.id : node.title, browse_secondary(node));
    }
    return rows;
}

std::optional<std::string> AppRuntimeContext::remoteBrowseTitleOverride() const
{
    if (state_.selected_remote_parent.kind != RemoteCatalogNodeKind::Root
        || !state_.selected_remote_parent.id.empty()
        || !state_.selected_remote_parent.title.empty()) {
        if (!state_.selected_remote_parent.title.empty()) {
            return state_.selected_remote_parent.title;
        }
        if (!state_.selected_remote_parent.id.empty()) {
            return state_.selected_remote_parent.id;
        }
    }
    if (const auto profile = selectedRemoteProfile()) {
        return remoteProfileLabel(*profile);
    }
    return std::nullopt;
}

std::optional<RemoteServerProfile> AppRuntimeContext::selectedRemoteProfile() const
{
    if (!state_.selected_remote_profile_index || *state_.selected_remote_profile_index >= state_.remote_profiles.size()) {
        return std::nullopt;
    }
    return state_.remote_profiles[*state_.selected_remote_profile_index];
}

RemoteServerProfile* AppRuntimeContext::selectedMutableRemoteProfile() noexcept
{
    if (!state_.selected_remote_profile_index || *state_.selected_remote_profile_index >= state_.remote_profiles.size()) {
        return nullptr;
    }
    return &state_.remote_profiles[*state_.selected_remote_profile_index];
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
    const auto profile = selectedRemoteProfile();
    if (!state_.selected_remote_stream) {
        return {{"STREAM", "NOT RESOLVED"}, {"ENTER", "RETRY"}};
    }
    const auto& diagnostics = state_.selected_remote_stream->diagnostics;
    NetworkBufferState buffer{};
    buffer.connected = diagnostics.connected;
    buffer.live = diagnostics.live;
    buffer.buffered_duration = diagnostics.live ? std::chrono::milliseconds{0} : buffer.minimum_playable_duration;
    const auto decision = StreamingPlaybackPolicy{}.bufferDecision(buffer);
    const auto recovery = StreamingPlaybackPolicy{}.recoveryPlan(buffer, !diagnostics.connected);
    return {
        {"SOURCE", diagnostics.source_name.empty() ? (profile ? remoteProfileLabel(*profile) : "REMOTE") : diagnostics.source_name},
        {"URL", diagnostics.resolved_url_redacted.empty() ? "REDACTED" : diagnostics.resolved_url_redacted},
        {"CONNECTION", diagnostics.connection_status.empty() ? (diagnostics.connected ? "READY" : "OFFLINE") : diagnostics.connection_status},
        {"BUFFER", bufferDecisionLabel(decision)},
        {"MIN BUFFER", std::to_string(static_cast<int>(buffer.minimum_playable_duration.count() / 1000)) + "S"},
        {"RECOVERY", recovery.retry ? (recovery.reconnect ? "RECONNECT" : "RETRY") : "NONE"},
        {"QUALITY", qualityLabel(state_.selected_remote_stream->quality_preference)},
        {"BITRATE", diagnostics.bitrate_kbps > 0 ? std::to_string(diagnostics.bitrate_kbps) + "K" : "UNKNOWN"},
        {"CODEC", diagnostics.codec.empty() ? "UNKNOWN" : diagnostics.codec},
        {"SAMPLE RATE", diagnostics.sample_rate_hz > 0 ? std::to_string(diagnostics.sample_rate_hz) + "HZ" : "UNKNOWN"},
        {"CHANNELS", diagnostics.channel_count > 0 ? std::to_string(diagnostics.channel_count) : "UNKNOWN"},
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
            rows.emplace_back(track->title, track->remote && !track->source_label.empty() ? track->source_label : "LOCAL");
        }
    } else if (!playback.current_stream_title.empty()) {
        rows.emplace_back(playback.current_stream_title, playback.current_stream_live ? "LIVE" : (playback.current_stream_source.empty() ? "STREAM" : playback.current_stream_source));
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
        rows.emplace_back(item.title.empty() ? item.id : item.title, item.source.source_label.empty() ? "LOCAL" : item.source.source_label);
    }

    if (const auto profile = selectedRemoteProfile(); profile && state_.selected_remote_session.available) {
        const auto remote_tracks = services_.remote.remote_catalog_provider->searchTracks(*profile, state_.selected_remote_session, state_.search_query, 4);
        for (const auto& track : remote_tracks) {
            rows.emplace_back(track.title.empty() ? track.id : track.title, remoteProfileLabel(*profile));
        }
    }

    if (rows.size() == 1) {
        rows.emplace_back("NO RESULTS", "EMPTY");
    }
    return rows;
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::remoteSetupRows() const
{
    std::vector<std::pair<std::string, std::string>> rows{};
    for (const auto& manifest : remote::RemoteSourceRegistry{}.manifests()) {
        const auto profile = std::find_if(state_.remote_profiles.begin(), state_.remote_profiles.end(), [&manifest](const RemoteServerProfile& candidate) {
            return candidate.kind == manifest.kind;
        });
        rows.emplace_back(
            manifest.display_name,
            profile == state_.remote_profiles.end() ? std::string{"ADD"} : remoteProfileReadiness(*profile));
    }
    return rows;
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::remoteProfileSettingsRows() const
{
    const auto profile = selectedRemoteProfile();
    if (!profile) {
        return {
            {"PROFILE", "ADD NEW"},
            {"TYPE", "JELLYFIN"},
            {"ADDRESS", "MISSING"},
            {"USER", "MISSING"},
            {"PASSWORD", "EMPTY"},
            {"SAVE", "ENTER"},
        };
    }

    return {
        {"PROFILE", remoteProfileLabel(*profile)},
        {"TYPE", remoteKindDisplayName(profile->kind)},
        {"LABEL", profile->name.empty() ? "MISSING" : profile->name},
        {"ADDRESS", profile->base_url.empty() ? "MISSING" : profile->base_url},
        {"USER", remoteUsernameLabel(*profile)},
        {"PASSWORD", remoteCredentialValueLabel(profile->kind, profile->password)},
        {"API TOKEN", remoteCredentialValueLabel(profile->kind, profile->api_token)},
        {"TLS VERIFY", profile->tls_policy.verify_peer ? "ON" : "OFF"},
        {"SELF SIGNED", profile->tls_policy.allow_self_signed ? "ALLOW" : "BLOCK"},
        {"PERMISSION", permissionLabel(profile->kind)},
        {"TEST", state_.remote_profile_status.empty() ? remoteProfileReadiness(*profile) : state_.remote_profile_status},
        {"SAVE", "PROFILE"},
    };
}

std::vector<std::pair<std::string, std::string>> AppRuntimeContext::remoteFieldEditorRows() const
{
    const bool secret = state_.remote_profile_edit_field == kRemoteProfileRowPassword
        || state_.remote_profile_edit_field == kRemoteProfileRowToken;
    return {
        {"FIELD", remoteEditFieldName(state_.remote_profile_edit_field)},
        {"VALUE", secret ? hiddenSecretLabel(state_.remote_profile_edit_buffer) : (state_.remote_profile_edit_buffer.empty() ? "EMPTY" : state_.remote_profile_edit_buffer)},
        {"ENTER", "SAVE"},
        {"BACK", "CANCEL"},
    };
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
    if (!state_.remote_browse_nodes.empty()) {
        rememberRemoteBrowse(*profile, state_.selected_remote_parent, state_.remote_browse_nodes);
    } else {
        state_.remote_browse_nodes = cachedRemoteBrowse(*profile, state_.selected_remote_parent);
    }
    if (state_.remote_browse_nodes.empty()) {
        state_.remote_browse_nodes = remote::RemoteCatalogMap::rootNodes();
    }
}

void AppRuntimeContext::ensureSelectedProfileCredentialRef(RemoteServerProfile& profile)
{
    if (profile.id.empty()) {
        profile.id = defaultRemoteProfileId(profile.kind, state_.remote_profiles.size());
    }
    if (!remoteProfileSupportsCredentials(profile.kind)) {
        profile.credential_ref.id.clear();
        return;
    }
    if (profile.credential_ref.id.empty()) {
        profile.credential_ref.id = "credential/" + profile.id;
    }
}

bool AppRuntimeContext::persistRemoteProfiles()
{
    for (auto& profile : state_.remote_profiles) {
        ensureSelectedProfileCredentialRef(profile);
    }
    const bool saved = services_.remote.remote_profile_store->saveProfiles(state_.remote_profiles);
    state_.remote_profile_status = saved ? "SAVED" : "SAVE FAIL";
    return saved;
}

void AppRuntimeContext::openRemoteSetup()
{
    state_.remote_profiles = services_.remote.remote_profile_store->loadProfiles();
    state_.remote_profile_status.clear();
    state_.navigation.list_selection.selected = 0;
    state_.navigation.list_selection.scroll = 0;
    commandPushPage(*this, AppPage::RemoteSetup);
}

void AppRuntimeContext::openRemoteProfileSettings(std::size_t profile_index, int focus_row)
{
    if (profile_index >= state_.remote_profiles.size()) {
        openNewRemoteProfileSettings(RemoteServerKind::Jellyfin);
        return;
    }
    state_.selected_remote_profile_index = profile_index;
    state_.selected_remote_kind = state_.remote_profiles[profile_index].kind;
    state_.remote_profile_status = remoteProfileReadiness(state_.remote_profiles[profile_index]);
    state_.navigation.list_selection.selected = std::max(0, focus_row);
    state_.navigation.list_selection.scroll = 0;
    commandPushPage(*this, AppPage::RemoteProfileSettings);
}

void AppRuntimeContext::openNewRemoteProfileSettings(RemoteServerKind kind)
{
    RemoteServerProfile profile{};
    profile.kind = kind;
    profile.id = defaultRemoteProfileId(kind, state_.remote_profiles.size());
    profile.name = defaultRemoteProfileName(kind);
    profile.tls_policy.verify_peer = true;
    ensureSelectedProfileCredentialRef(profile);
    state_.remote_profiles.push_back(std::move(profile));
    openRemoteProfileSettings(state_.remote_profiles.size() - 1U, kRemoteProfileRowKind);
}

void AppRuntimeContext::beginRemoteProfileFieldEdit(int field)
{
    auto* profile = selectedMutableRemoteProfile();
    if (profile == nullptr) {
        openNewRemoteProfileSettings(RemoteServerKind::Jellyfin);
        profile = selectedMutableRemoteProfile();
    }
    if (profile == nullptr) {
        return;
    }

    state_.remote_profile_edit_field = field;
    switch (field) {
    case kRemoteProfileRowName:
        state_.remote_profile_edit_buffer = profile->name;
        break;
    case kRemoteProfileRowAddress:
        state_.remote_profile_edit_buffer = profile->base_url;
        break;
    case kRemoteProfileRowUsername:
        state_.remote_profile_edit_buffer = profile->username;
        break;
    case kRemoteProfileRowPassword:
    case kRemoteProfileRowToken:
        state_.remote_profile_edit_buffer.clear();
        break;
    default:
        return;
    }
    commandPushPage(*this, AppPage::RemoteFieldEditor);
}

bool AppRuntimeContext::handleSettingsRemoteConfirm(int selected)
{
    (void)selected;
    openRemoteSetup();
    return true;
}

bool AppRuntimeContext::handleRemoteSetupConfirm(int selected)
{
    const auto manifests = remote::RemoteSourceRegistry{}.manifests();
    if (selected < 0 || selected >= static_cast<int>(manifests.size())) {
        return false;
    }

    const auto kind = manifests[static_cast<std::size_t>(selected)].kind;
    const auto it = std::find_if(state_.remote_profiles.begin(), state_.remote_profiles.end(), [kind](const RemoteServerProfile& profile) {
        return profile.kind == kind;
    });
    if (it != state_.remote_profiles.end()) {
        openRemoteProfileSettings(static_cast<std::size_t>(std::distance(state_.remote_profiles.begin(), it)), kRemoteProfileRowAddress);
    } else {
        openNewRemoteProfileSettings(kind);
    }
    return true;
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
            openRemoteProfileSettings(static_cast<std::size_t>(std::distance(state_.remote_profiles.begin(), it)), kRemoteProfileRowAddress);
        } else {
            openNewRemoteProfileSettings(wanted);
        }
        return true;
    }

    const int manifest_index = selected - 3;
    if (manifest_index >= 0 && manifest_index < static_cast<int>(manifests.size())) {
        openNewRemoteProfileSettings(manifests[static_cast<std::size_t>(manifest_index)].kind);
        return true;
    }
    return false;
}

bool AppRuntimeContext::handleRemoteProfileSettingsConfirm(int selected)
{
    auto* profile = selectedMutableRemoteProfile();
    if (profile == nullptr) {
        openNewRemoteProfileSettings(RemoteServerKind::Jellyfin);
        return true;
    }

    switch (selected) {
    case kRemoteProfileRowProfile:
        if (!state_.remote_profiles.empty()) {
            const auto current = state_.selected_remote_profile_index.value_or(0U);
            const auto next = (current + 1U) % state_.remote_profiles.size();
            openRemoteProfileSettings(next, kRemoteProfileRowProfile);
        }
        return true;
    case kRemoteProfileRowKind:
        openRemoteSetup();
        return true;
    case kRemoteProfileRowName:
    case kRemoteProfileRowAddress:
    case kRemoteProfileRowUsername:
    case kRemoteProfileRowPassword:
    case kRemoteProfileRowToken:
        beginRemoteProfileFieldEdit(selected);
        return true;
    case kRemoteProfileRowTlsVerify:
        profile->tls_policy.verify_peer = !profile->tls_policy.verify_peer;
        if (profile->tls_policy.verify_peer) {
            profile->tls_policy.allow_self_signed = false;
        }
        (void)persistRemoteProfiles();
        return true;
    case kRemoteProfileRowSelfSigned:
        profile->tls_policy.allow_self_signed = !profile->tls_policy.allow_self_signed;
        if (profile->tls_policy.allow_self_signed) {
            profile->tls_policy.verify_peer = false;
        }
        (void)persistRemoteProfiles();
        return true;
    case kRemoteProfileRowPermission:
        state_.remote_profile_status = permissionLabel(profile->kind);
        return true;
    case kRemoteProfileRowTest:
        state_.selected_remote_session = services_.remote.remote_source_provider->probe(*profile);
        state_.remote_profile_status = state_.selected_remote_session.available ? "ONLINE" : "FAILED";
        return true;
    case kRemoteProfileRowSave:
        (void)persistRemoteProfiles();
        if (!profile->password.empty() || !profile->api_token.empty()) {
            ensureSelectedProfileCredentialRef(*profile);
            (void)services_.remote.remote_profile_store->saveCredentials(*profile);
        }
        return true;
    default:
        return false;
    }
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
    if (services_.cache.cache_manager) {
        (void)services_.cache.cache_manager->rememberRecentBrowse(
            profile->id,
            node.id,
            node.title.empty() ? node.id : node.title);
    }
    if (node.playable) {
        const auto track = cachedRemoteTrackFromNode(*profile, node);
        rememberRemoteTrackFacts(*profile, track);
        std::vector<RemoteTrack> visible_tracks{};
        visible_tracks.reserve(state_.remote_browse_nodes.size());
        for (const auto& visible_node : state_.remote_browse_nodes) {
            if (visible_node.playable) {
                visible_tracks.push_back(cachedRemoteTrackFromNode(*profile, visible_node));
            }
        }
        if (visible_tracks.empty()) {
            visible_tracks.push_back(track);
        }
        controllers_.library.mergeRemoteTracks(*profile, visible_tracks);

        std::vector<int> ids{};
        ids.reserve(visible_tracks.size());
        int selected_track_id = 0;
        for (const auto& visible_track : visible_tracks) {
            for (const auto& library_track : controllers_.library.model().tracks) {
                if (library_track.remote
                    && library_track.remote_profile_id == profile->id
                    && library_track.remote_track_id == visible_track.id) {
                    ids.push_back(library_track.id);
                    if (visible_track.id == track.id) {
                        selected_track_id = library_track.id;
                    }
                    break;
                }
            }
        }
        if (!ids.empty()) {
            controllers_.library.setSongsContextTrackIds(
                state_.selected_remote_parent.title.empty() ? std::string{"SONGS"} : state_.selected_remote_parent.title,
                ids);
        }
        if (selected_track_id != 0 && startLibraryTrack(selected_track_id)) {
            commandPushPage(*this, AppPage::NowPlaying);
        }
        return true;
    }

    if (node.browsable) {
        state_.selected_remote_parent = node;
        state_.remote_browse_nodes = services_.remote.remote_catalog_provider->browse(*profile, state_.selected_remote_session, node, 100);
        if (!state_.remote_browse_nodes.empty()) {
            rememberRemoteBrowse(*profile, node, state_.remote_browse_nodes);
        } else {
            state_.remote_browse_nodes = cachedRemoteBrowse(*profile, node);
        }
        state_.navigation.list_selection.selected = 0;
        state_.navigation.list_selection.scroll = 0;
        return true;
    }
    return false;
}

bool AppRuntimeContext::handleStreamDetailConfirm()
{
    const auto profile = selectedRemoteProfile();
    if (!profile) {
        return false;
    }
    if (!state_.selected_remote_stream) {
        if (!state_.selected_remote_node) {
            return false;
        }
        const auto track = cachedRemoteTrackFromNode(*profile, *state_.selected_remote_node);
        state_.selected_remote_stream = services_.remote.remote_stream_resolver->resolveTrack(*profile, state_.selected_remote_session, track);
    }
    if (!state_.selected_remote_stream) {
        return false;
    }
    const auto track = state_.selected_remote_node ? cachedRemoteTrackFromNode(*profile, *state_.selected_remote_node) : RemoteTrack{};
    const auto source = remoteProfileLabel(*profile);
    return state_.selected_remote_stream ? controllers_.playback.startRemoteStream(*state_.selected_remote_stream, track, source) : false;
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
            return item.local_track_id ? startLibraryTrack(*item.local_track_id) : false;
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
            RemoteCatalogNode candidate{};
            candidate.kind = RemoteCatalogNodeKind::Tracks;
            candidate.id = track.id;
            candidate.title = track.title;
            candidate.artist = track.artist;
            candidate.album = track.album;
            candidate.album_id = track.album_id;
            candidate.duration_seconds = track.duration_seconds;
            candidate.playable = true;
            candidate.browsable = false;
            const auto cached = cachedRemoteTrackFromNode(*profile, candidate);
            const auto enriched_track = mergeRemoteTrackCache(track, cached);
            rememberRemoteTrackFacts(*profile, enriched_track);
            state_.selected_remote_stream = services_.remote.remote_stream_resolver->resolveTrack(*profile, state_.selected_remote_session, enriched_track);
            RemoteCatalogNode selected_node{};
            selected_node.kind = RemoteCatalogNodeKind::Tracks;
            selected_node.id = enriched_track.id;
            selected_node.title = enriched_track.title;
            selected_node.subtitle = enriched_track.artist;
            selected_node.playable = true;
            selected_node.browsable = false;
            selected_node.artist = enriched_track.artist;
            selected_node.album = enriched_track.album;
            selected_node.duration_seconds = enriched_track.duration_seconds;
            selected_node.album_id = enriched_track.album_id;
            selected_node.source_id = enriched_track.source_id;
            selected_node.source_label = enriched_track.source_label;
            selected_node.artwork_key = enriched_track.artwork_key;
            selected_node.artwork_url = enriched_track.artwork_url;
            selected_node.lyrics_plain = enriched_track.lyrics_plain;
            selected_node.lyrics_synced = enriched_track.lyrics_synced;
            selected_node.lyrics_source = enriched_track.lyrics_source;
            selected_node.fingerprint = enriched_track.fingerprint;
            state_.selected_remote_node = selected_node;
            return state_.selected_remote_stream
                ? controllers_.playback.startRemoteStream(*state_.selected_remote_stream, enriched_track, remoteProfileLabel(*profile))
                : false;
        }
        ++cursor;
    }
    return false;
}

RemoteTrack AppRuntimeContext::cachedRemoteTrackFromNode(const RemoteServerProfile& profile, const RemoteCatalogNode& node) const
{
    auto track = remoteTrackFromCatalogNode(node);
    if (track.source_id.empty()) {
        track.source_id = profile.id;
    }
    if (track.source_label.empty()) {
        track.source_label = remoteProfileLabel(profile);
    }
    if (!services_.cache.cache_manager || node.id.empty()) {
        return track;
    }
    const auto cached = services_.cache.cache_manager->getText(
        ::lofibox::cache::CacheBucket::Metadata,
        remoteMediaCacheKey(profile, node.id));
    return cached ? mergeRemoteTrackCache(std::move(track), parseRemoteTrackCache(*cached)) : track;
}

RemoteTrack AppRuntimeContext::remoteTrackFromLibraryTrack(const RemoteServerProfile& profile, const TrackRecord& track) const
{
    RemoteTrack remote_track{};
    remote_track.id = track.remote_track_id;
    remote_track.title = track.title;
    remote_track.artist = track.artist;
    remote_track.album = track.album;
    remote_track.duration_seconds = track.duration_seconds;
    remote_track.source_id = track.remote_profile_id.empty() ? profile.id : track.remote_profile_id;
    remote_track.source_label = track.source_label.empty()
        ? remoteProfileLabel(profile)
        : track.source_label;
    remote_track.artwork_key = track.artwork_key;
    remote_track.artwork_url = track.artwork_url;
    remote_track.lyrics_plain = track.lyrics_plain;
    remote_track.lyrics_synced = track.lyrics_synced;
    remote_track.lyrics_source = track.lyrics_source;
    remote_track.fingerprint = track.fingerprint;

    if (services_.cache.cache_manager && !remote_track.id.empty()) {
        const auto cached = services_.cache.cache_manager->getText(
            ::lofibox::cache::CacheBucket::Metadata,
            remoteMediaCacheKey(profile, remote_track.id));
        if (cached) {
            remote_track = preferCachedRemoteTrackFacts(std::move(remote_track), parseRemoteTrackCache(*cached));
        }
    }
    return remote_track;
}

bool AppRuntimeContext::startRemoteLibraryTrack(const TrackRecord& track)
{
    if (!track.remote || track.remote_profile_id.empty() || track.remote_track_id.empty()) {
        return false;
    }

    const auto profile_it = std::find_if(state_.remote_profiles.begin(), state_.remote_profiles.end(), [&track](const RemoteServerProfile& profile) {
        return profile.id == track.remote_profile_id;
    });
    if (profile_it == state_.remote_profiles.end()) {
        return false;
    }

    const auto profile_index = static_cast<std::size_t>(std::distance(state_.remote_profiles.begin(), profile_it));
    state_.selected_remote_profile_index = profile_index;
    state_.selected_remote_kind = profile_it->kind;
    state_.selected_remote_session = services_.remote.remote_source_provider->probe(*profile_it);
    if (!state_.selected_remote_session.available) {
        return false;
    }

    auto remote_track = remoteTrackFromLibraryTrack(*profile_it, track);
    if (remote_track.source_label.empty()) {
        remote_track.source_label = remoteProfileLabel(*profile_it);
    }
    rememberRemoteTrackFacts(*profile_it, remote_track);
    state_.selected_remote_node = RemoteCatalogNode{
        RemoteCatalogNodeKind::Tracks,
        remote_track.id,
        remote_track.title,
        remote_track.artist,
        true,
        false,
        remote_track.artist,
        remote_track.album,
        remote_track.duration_seconds,
        remote_track.album_id,
        remote_track.source_id,
        remote_track.source_label,
        remote_track.artwork_key,
        remote_track.artwork_url,
        remote_track.lyrics_plain,
        remote_track.lyrics_synced,
        remote_track.lyrics_source,
        remote_track.fingerprint};
    state_.selected_remote_stream = services_.remote.remote_stream_resolver->resolveTrack(*profile_it, state_.selected_remote_session, remote_track);
    auto* mutable_track = controllers_.library.findMutableTrack(track.id);
    if (!state_.selected_remote_stream || mutable_track == nullptr) {
        return false;
    }

    const auto source = !remote_track.source_label.empty()
        ? remote_track.source_label
        : remoteProfileLabel(*profile_it);
    return controllers_.playback.startRemoteLibraryTrack(
        *state_.selected_remote_stream,
        *mutable_track,
        *profile_it,
        remote_track,
        source,
        remoteProfileKeepsLocalFacts(profile_it->kind));
}

void AppRuntimeContext::rememberRemoteTrackFacts(const RemoteServerProfile& profile, const RemoteTrack& track) const
{
    if (!services_.cache.cache_manager || track.id.empty() || !remoteTrackHasLocalCacheableFacts(track)) {
        return;
    }

    if (!remoteProfileKeepsLocalFacts(profile.kind)) {
        return;
    }

    (void)services_.cache.cache_manager->putText(
        ::lofibox::cache::CacheBucket::Metadata,
        remoteMediaCacheKey(profile, track.id),
        serializeRemoteTrackCache(track));

    if (!track.lyrics_plain.empty() || !track.lyrics_synced.empty()) {
        (void)services_.cache.cache_manager->putText(
            ::lofibox::cache::CacheBucket::Lyrics,
            remoteMediaCacheKey(profile, track.id),
            serializeRemoteTrackCache(track));
    }
}

void AppRuntimeContext::rememberRemoteBrowse(const RemoteServerProfile& profile, const RemoteCatalogNode& parent, const std::vector<RemoteCatalogNode>& nodes) const
{
    if (!services_.cache.cache_manager || nodes.empty()) {
        return;
    }
    (void)services_.cache.cache_manager->rememberRemoteDirectory(
        profile.id,
        remoteDirectoryCacheKey(profile, parent),
        remoteBrowseCachePayload(nodes));
}

std::vector<RemoteCatalogNode> AppRuntimeContext::cachedRemoteBrowse(const RemoteServerProfile& profile, const RemoteCatalogNode& parent) const
{
    if (!services_.cache.cache_manager) {
        return {};
    }
    const auto cached = services_.cache.cache_manager->remoteDirectory(
        profile.id,
        remoteDirectoryCacheKey(profile, parent));
    return cached ? parseRemoteBrowseCachePayload(*cached) : std::vector<RemoteCatalogNode>{};
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

void AppRuntimeContext::appendRemoteProfileEditCharacter(char ch)
{
    if (state_.remote_profile_edit_buffer.size() < 256U) {
        state_.remote_profile_edit_buffer.push_back(ch);
    }
}

void AppRuntimeContext::backspaceRemoteProfileEdit()
{
    if (!state_.remote_profile_edit_buffer.empty()) {
        state_.remote_profile_edit_buffer.pop_back();
    }
}

void AppRuntimeContext::commitRemoteProfileEdit()
{
    auto* profile = selectedMutableRemoteProfile();
    if (profile == nullptr) {
        commandPopPage(*this);
        return;
    }

    switch (state_.remote_profile_edit_field) {
    case kRemoteProfileRowName:
        profile->name = state_.remote_profile_edit_buffer;
        break;
    case kRemoteProfileRowAddress:
        profile->base_url = state_.remote_profile_edit_buffer;
        break;
    case kRemoteProfileRowUsername:
        profile->username = state_.remote_profile_edit_buffer;
        break;
    case kRemoteProfileRowPassword:
        ensureSelectedProfileCredentialRef(*profile);
        profile->password = state_.remote_profile_edit_buffer;
        (void)services_.remote.remote_profile_store->saveCredentials(*profile);
        break;
    case kRemoteProfileRowToken:
        ensureSelectedProfileCredentialRef(*profile);
        profile->api_token = state_.remote_profile_edit_buffer;
        (void)services_.remote.remote_profile_store->saveCredentials(*profile);
        break;
    default:
        break;
    }

    state_.remote_profile_edit_field = -1;
    state_.remote_profile_edit_buffer.clear();
    (void)persistRemoteProfiles();
    state_.remote_profile_status = remoteProfileReadiness(*profile);
    commandPopPage(*this);
}

} // namespace lofibox::app
