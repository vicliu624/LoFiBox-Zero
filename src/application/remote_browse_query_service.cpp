// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/remote_browse_query_service.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

#include "app/remote_media_contract.h"
#include "application/source_profile_command_service.h"
#include "cache/cache_manager.h"
#include "playback/streaming_playback_policy.h"
#include "remote/common/remote_catalog_model.h"
#include "remote/common/remote_provider_contract.h"

namespace lofibox::application {
namespace {

std::string bufferDecisionLabel(app::BufferDecision decision)
{
    switch (decision) {
    case app::BufferDecision::StartImmediately: return "READY";
    case app::BufferDecision::WaitForMinimumBuffer: return "BUFFERING";
    case app::BufferDecision::PauseAndRebuffer: return "REBUFFER";
    }
    return "UNKNOWN";
}

std::string recoveryLabel(const app::StreamingRecoveryPlan& plan)
{
    if (!plan.retry) {
        return "NONE";
    }
    return plan.reconnect ? "RECONNECT" : "RETRY";
}

app::RemoteTrack preferCachedRemoteTrackFacts(app::RemoteTrack current, const app::RemoteTrack& cached)
{
    const auto missingText = [](std::string_view value) {
        return value.empty() || value == "-" || value == "UNKNOWN";
    };
    const auto missingTitle = [&current, &missingText]() {
        return missingText(current.title) || (!current.id.empty() && current.title == current.id);
    };

    if (missingTitle() && !cached.title.empty()) current.title = cached.title;
    if (missingText(current.artist) && !cached.artist.empty()) current.artist = cached.artist;
    if (missingText(current.album) && !cached.album.empty()) current.album = cached.album;
    if (current.album_id.empty() && !cached.album_id.empty()) current.album_id = cached.album_id;
    if (cached.duration_seconds > 0) current.duration_seconds = cached.duration_seconds;
    if (current.source_id.empty() && !cached.source_id.empty()) current.source_id = cached.source_id;
    if (current.source_label.empty() && !cached.source_label.empty()) current.source_label = cached.source_label;
    if (current.artwork_key.empty() && !cached.artwork_key.empty()) current.artwork_key = cached.artwork_key;
    if (current.artwork_url.empty() && !cached.artwork_url.empty()) current.artwork_url = cached.artwork_url;
    if (current.lyrics_plain.empty() && !cached.lyrics_plain.empty()) current.lyrics_plain = cached.lyrics_plain;
    if (current.lyrics_synced.empty() && !cached.lyrics_synced.empty()) current.lyrics_synced = cached.lyrics_synced;
    if (current.lyrics_source.empty() && !cached.lyrics_source.empty()) current.lyrics_source = cached.lyrics_source;
    if (current.fingerprint.empty() && !cached.fingerprint.empty()) current.fingerprint = cached.fingerprint;
    return current;
}

std::string capabilityName(remote::RemoteProviderCapability capability)
{
    switch (capability) {
    case remote::RemoteProviderCapability::Probe: return "probe";
    case remote::RemoteProviderCapability::SearchTracks: return "search";
    case remote::RemoteProviderCapability::RecentTracks: return "recent";
    case remote::RemoteProviderCapability::ResolveStream: return "resolve";
    case remote::RemoteProviderCapability::BrowseCatalog: return "browse";
    case remote::RemoteProviderCapability::BrowseArtists: return "artists";
    case remote::RemoteProviderCapability::BrowseAlbums: return "albums";
    case remote::RemoteProviderCapability::BrowsePlaylists: return "playlists";
    case remote::RemoteProviderCapability::BrowseFolders: return "folders";
    case remote::RemoteProviderCapability::BrowseGenres: return "genres";
    case remote::RemoteProviderCapability::Favorites: return "favorites";
    case remote::RemoteProviderCapability::DirectUrlPlayback: return "direct-url";
    case remote::RemoteProviderCapability::PlaylistManifestPlayback: return "playlist";
    case remote::RemoteProviderCapability::InternetRadioPlayback: return "radio";
    case remote::RemoteProviderCapability::AdaptiveStreaming: return "adaptive";
    case remote::RemoteProviderCapability::LanShareAccess: return "lan-share";
    case remote::RemoteProviderCapability::DlnaDiscovery: return "dlna";
    case remote::RemoteProviderCapability::QualitySelection: return "quality";
    case remote::RemoteProviderCapability::TranscodingPolicy: return "transcode";
    case remote::RemoteProviderCapability::WritableFavorites: return "writable-favorites";
    case remote::RemoteProviderCapability::WritableMetadata: return "writable-metadata";
    case remote::RemoteProviderCapability::ReadOnly: return "read-only";
    }
    return "unknown";
}

bool manifestWritable(const remote::RemoteProviderManifest& manifest) noexcept
{
    return remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::WritableMetadata)
        || remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::WritableFavorites);
}

} // namespace

RemoteBrowseQueryService::RemoteBrowseQueryService(const app::RuntimeServices& services) noexcept
    : services_(services)
{
}

RemoteBrowseQueryResult RemoteBrowseQueryService::browseRoot(app::RemoteServerProfile& profile, std::size_t profile_count, int limit) const
{
    app::RemoteCatalogNode root{};
    auto result = browseChild(profile, SourceProfileCommandService{services_}.probe(profile, profile_count).session, root, limit);
    if (result.nodes.empty()) {
        result.nodes = remote::RemoteCatalogMap::rootNodes();
        result.degraded = true;
        result.degraded_reason = result.degraded_reason.empty() ? "ROOT FALLBACK" : result.degraded_reason;
    }
    return result;
}

RemoteBrowseQueryResult RemoteBrowseQueryService::browseChild(
    app::RemoteServerProfile& profile,
    const app::RemoteSourceSession& session,
    const app::RemoteCatalogNode& parent,
    int limit) const
{
    RemoteBrowseQueryResult result{};
    result.profile = profile;
    result.session = session;
    result.parent = parent;
    result.source_label = sourceLabel(profile);
    result.online = session.available;

    if (!session.available) {
        result.nodes = cachedBrowse(profile, parent);
        if (result.nodes.empty() && parent.kind == app::RemoteCatalogNodeKind::Root && parent.id.empty()) {
            result.nodes = remote::RemoteCatalogMap::rootNodes();
        }
        result.from_cache = !result.nodes.empty();
        result.degraded = true;
        result.degraded_reason = session.message.empty() ? "OFFLINE" : session.message;
        result.command = result.nodes.empty()
            ? CommandResult::failure("remote-browse.offline", result.degraded_reason)
            : CommandResult::success("remote-browse.cached", "CACHED");
    } else if (!services_.remote.remote_catalog_provider->available()) {
        result.nodes = cachedBrowse(profile, parent);
        result.from_cache = !result.nodes.empty();
        result.degraded = true;
        result.degraded_reason = "CATALOG PROVIDER UNAVAILABLE";
        result.command = result.nodes.empty()
            ? CommandResult::failure("remote-browse.provider-unavailable", result.degraded_reason)
            : CommandResult::success("remote-browse.cached", "CACHED");
    } else {
        result.nodes = services_.remote.remote_catalog_provider->browse(profile, session, parent, limit);
        if (!result.nodes.empty()) {
            rememberBrowse(profile, parent, result.nodes);
            result.command = CommandResult::success("remote-browse.ok", "OK");
        } else {
            result.nodes = cachedBrowse(profile, parent);
            result.from_cache = !result.nodes.empty();
            result.degraded = result.from_cache;
            result.degraded_reason = result.from_cache ? "REMOTE EMPTY; USING CACHE" : "EMPTY";
            result.command = result.from_cache
                ? CommandResult::success("remote-browse.cached", "CACHED")
                : CommandResult::success("remote-browse.empty", "EMPTY");
        }
    }

    result.playable_tracks = tracksFromPlayableNodes(profile, result.nodes);
    return result;
}

RemoteBrowseQueryResult RemoteBrowseQueryService::libraryTracks(app::RemoteServerProfile& profile, std::size_t profile_count, int limit) const
{
    RemoteBrowseQueryResult result{};
    result.profile = profile;
    result.source_label = sourceLabel(profile);
    result.session = SourceProfileCommandService{services_}.probe(profile, profile_count).session;
    result.online = result.session.available;
    if (!result.session.available) {
        result.degraded = true;
        result.degraded_reason = result.session.message.empty() ? "OFFLINE" : result.session.message;
        result.command = CommandResult::failure("remote-library.offline", result.degraded_reason);
        return result;
    }
    if (!services_.remote.remote_catalog_provider->available()) {
        result.degraded = true;
        result.degraded_reason = "CATALOG PROVIDER UNAVAILABLE";
        result.command = CommandResult::failure("remote-library.provider-unavailable", result.degraded_reason);
        return result;
    }

    result.playable_tracks = services_.remote.remote_catalog_provider->libraryTracks(profile, result.session, limit);
    for (auto& track : result.playable_tracks) {
        if (track.source_id.empty()) {
            track.source_id = profile.id;
        }
        if (track.source_label.empty()) {
            track.source_label = result.source_label;
        }
        track = mergeCachedTrackFacts(profile, std::move(track));
        rememberTrackFacts(profile, track);
    }
    result.command = CommandResult::success("remote-library.ok", "OK");
    return result;
}

RemoteSearchQueryResult RemoteBrowseQueryService::search(app::RemoteServerProfile& profile, std::size_t profile_count, std::string_view query, int limit) const
{
    RemoteSearchQueryResult result{};
    result.profile = profile;
    result.source_label = sourceLabel(profile);
    result.session = SourceProfileCommandService{services_}.probe(profile, profile_count).session;
    if (!result.session.available) {
        result.degraded = true;
        result.degraded_reason = result.session.message.empty() ? "OFFLINE" : result.session.message;
        result.command = CommandResult::failure("remote-search.offline", result.degraded_reason);
        return result;
    }
    if (!services_.remote.remote_catalog_provider->available()) {
        result.degraded = true;
        result.degraded_reason = "CATALOG PROVIDER UNAVAILABLE";
        result.command = CommandResult::failure("remote-search.provider-unavailable", result.degraded_reason);
        return result;
    }

    result.tracks = services_.remote.remote_catalog_provider->searchTracks(profile, result.session, query, limit);
    for (auto& track : result.tracks) {
        if (track.source_id.empty()) {
            track.source_id = profile.id;
        }
        if (track.source_label.empty()) {
            track.source_label = result.source_label;
        }
        track = mergeCachedTrackFacts(profile, std::move(track));
        rememberTrackFacts(profile, track);
    }
    result.command = CommandResult::success("remote-search.ok", "OK");
    return result;
}

RemoteResolveQueryResult RemoteBrowseQueryService::resolve(
    app::RemoteServerProfile& profile,
    const app::RemoteSourceSession& session,
    const app::RemoteTrack& track) const
{
    RemoteResolveQueryResult result{};
    result.profile = profile;
    result.session = session;
    result.track = mergeCachedTrackFacts(profile, track);
    if (result.track.source_id.empty()) {
        result.track.source_id = profile.id;
    }
    if (result.track.source_label.empty()) {
        result.track.source_label = sourceLabel(profile);
    }
    rememberTrackFacts(profile, result.track);

    if (!session.available) {
        result.command = CommandResult::failure("remote-resolve.offline", session.message.empty() ? "OFFLINE" : session.message);
        return result;
    }
    if (!services_.remote.remote_stream_resolver->available()) {
        result.command = CommandResult::failure("remote-resolve.provider-unavailable", "STREAM RESOLVER UNAVAILABLE");
        return result;
    }
    result.stream = services_.remote.remote_stream_resolver->resolveTrack(profile, session, result.track);
    result.command = result.stream
        ? CommandResult::success("remote-resolve.ok", "OK")
        : CommandResult::failure("remote-resolve.failed", "NOT RESOLVED");
    return result;
}

app::RemoteTrack RemoteBrowseQueryService::trackFromNode(const app::RemoteServerProfile& profile, const app::RemoteCatalogNode& node) const
{
    auto track = app::remoteTrackFromCatalogNode(node);
    if (track.source_id.empty()) {
        track.source_id = profile.id;
    }
    if (track.source_label.empty()) {
        track.source_label = sourceLabel(profile);
    }
    return mergeCachedTrackFacts(profile, std::move(track));
}

std::vector<app::RemoteTrack> RemoteBrowseQueryService::tracksFromPlayableNodes(
    const app::RemoteServerProfile& profile,
    const std::vector<app::RemoteCatalogNode>& nodes) const
{
    std::vector<app::RemoteTrack> tracks{};
    tracks.reserve(nodes.size());
    for (const auto& node : nodes) {
        if (node.playable) {
            tracks.push_back(trackFromNode(profile, node));
        }
    }
    return tracks;
}

void RemoteBrowseQueryService::rememberRecentBrowse(const app::RemoteServerProfile& profile, const app::RemoteCatalogNode& node) const
{
    if (!services_.cache.cache_manager) {
        return;
    }
    (void)services_.cache.cache_manager->rememberRecentBrowse(
        profile.id,
        node.id,
        node.title.empty() ? node.id : node.title);
}

void RemoteBrowseQueryService::rememberTrackFacts(const app::RemoteServerProfile& profile, const app::RemoteTrack& track) const
{
    if (!services_.cache.cache_manager || track.id.empty() || !app::remoteTrackHasLocalCacheableFacts(track)) {
        return;
    }
    if (!keepsLocalFacts(profile.kind)) {
        return;
    }

    (void)services_.cache.cache_manager->putText(
        ::lofibox::cache::CacheBucket::Metadata,
        app::remoteMediaCacheKey(profile, track.id),
        app::serializeRemoteTrackCache(track));

    if (!track.lyrics_plain.empty() || !track.lyrics_synced.empty()) {
        (void)services_.cache.cache_manager->putText(
            ::lofibox::cache::CacheBucket::Lyrics,
            app::remoteMediaCacheKey(profile, track.id),
            app::serializeRemoteTrackCache(track));
    }
}

RemoteSourceDiagnostics RemoteBrowseQueryService::sourceDiagnostics(
    const std::optional<app::RemoteServerProfile>& profile,
    const app::RemoteSourceSession& session) const
{
    RemoteSourceDiagnostics diagnostics{};
    diagnostics.provider_available = services_.remote.remote_source_provider->available();
    diagnostics.catalog_available = services_.remote.remote_catalog_provider->available();
    diagnostics.resolver_available = services_.remote.remote_stream_resolver->available();
    diagnostics.session_available = session.available;
    diagnostics.connection_status = session.available ? "ONLINE" : "OFFLINE";
    diagnostics.message = session.message.empty() ? "-" : session.message;
    diagnostics.token_status = session.access_token.empty() ? "NONE" : "REDACTED";

    if (!profile) {
        diagnostics.source_label = "NOT SELECTED";
        diagnostics.kind_id = "unknown";
        diagnostics.family = "unknown";
        diagnostics.user = "-";
        diagnostics.credential_status = "NONE";
        diagnostics.tls_status = "UNKNOWN";
        diagnostics.permission = "UNKNOWN";
        return diagnostics;
    }

    const auto manifest = remote::remoteProviderManifest(profile->kind);
    diagnostics.source_label = sourceLabel(*profile);
    diagnostics.kind_id = manifest.type_id;
    diagnostics.family = manifest.family;
    diagnostics.user = profile->username.empty() ? "-" : profile->username;
    diagnostics.credential_attached = !profile->credential_ref.id.empty();
    diagnostics.credential_status = diagnostics.credential_attached ? "XDG STATE" : "NONE";
    diagnostics.tls_status = profile->tls_policy.verify_peer ? "VERIFY" : "EXCEPTION";
    diagnostics.permission = manifestWritable(manifest) ? "READ/WRITE" : "READ ONLY";
    diagnostics.capabilities.reserve(manifest.capabilities.size());
    for (const auto capability : manifest.capabilities) {
        diagnostics.capabilities.push_back(capabilityName(capability));
    }
    return diagnostics;
}

RemoteStreamDiagnosticsQuery RemoteBrowseQueryService::streamDiagnostics(
    const std::optional<app::RemoteServerProfile>& profile,
    const std::optional<app::ResolvedRemoteStream>& stream) const
{
    RemoteStreamDiagnosticsQuery result{};
    if (!stream) {
        return result;
    }
    result.resolved = true;
    const auto& diagnostics = stream->diagnostics;
    app::NetworkBufferState buffer{};
    buffer.connected = diagnostics.connected;
    buffer.live = diagnostics.live;
    buffer.buffered_duration = diagnostics.live ? std::chrono::milliseconds{0} : buffer.minimum_playable_duration;
    const auto decision = app::StreamingPlaybackPolicy{}.bufferDecision(buffer);
    const auto recovery = app::StreamingPlaybackPolicy{}.recoveryPlan(buffer, !diagnostics.connected);

    result.source_name = diagnostics.source_name.empty()
        ? (profile ? sourceLabel(*profile) : std::string{"REMOTE"})
        : diagnostics.source_name;
    result.redacted_url = diagnostics.resolved_url_redacted.empty() ? "REDACTED" : diagnostics.resolved_url_redacted;
    result.connection_status = diagnostics.connection_status.empty()
        ? (diagnostics.connected ? "READY" : "OFFLINE")
        : diagnostics.connection_status;
    result.buffer_state = bufferDecisionLabel(decision);
    result.minimum_playable_seconds = static_cast<int>(buffer.minimum_playable_duration.count() / 1000);
    result.recovery_action = recoveryLabel(recovery);
    result.quality_preference = stream->quality_preference;
    result.bitrate_kbps = diagnostics.bitrate_kbps;
    result.codec = diagnostics.codec;
    result.sample_rate_hz = diagnostics.sample_rate_hz;
    result.channel_count = diagnostics.channel_count;
    result.live = diagnostics.live;
    result.seekable = diagnostics.seekable;
    return result;
}

std::string RemoteBrowseQueryService::sourceLabel(const app::RemoteServerProfile& profile) const
{
    if (!profile.name.empty()) {
        return profile.name;
    }
    if (!profile.base_url.empty()) {
        return profile.base_url;
    }
    return SourceProfileCommandService{services_}.defaultProfileName(profile.kind);
}

bool RemoteBrowseQueryService::keepsLocalFacts(app::RemoteServerKind kind) const
{
    return SourceProfileCommandService{services_}.keepsLocalFacts(kind);
}

std::vector<app::RemoteCatalogNode> RemoteBrowseQueryService::cachedBrowse(
    const app::RemoteServerProfile& profile,
    const app::RemoteCatalogNode& parent) const
{
    if (!services_.cache.cache_manager) {
        return {};
    }
    const auto cached = services_.cache.cache_manager->remoteDirectory(
        profile.id,
        app::remoteDirectoryCacheKey(profile, parent));
    return cached ? app::parseRemoteBrowseCachePayload(*cached) : std::vector<app::RemoteCatalogNode>{};
}

void RemoteBrowseQueryService::rememberBrowse(
    const app::RemoteServerProfile& profile,
    const app::RemoteCatalogNode& parent,
    const std::vector<app::RemoteCatalogNode>& nodes) const
{
    if (!services_.cache.cache_manager || nodes.empty()) {
        return;
    }
    (void)services_.cache.cache_manager->rememberRemoteDirectory(
        profile.id,
        app::remoteDirectoryCacheKey(profile, parent),
        app::remoteBrowseCachePayload(nodes));
}

app::RemoteTrack RemoteBrowseQueryService::mergeCachedTrackFacts(
    const app::RemoteServerProfile& profile,
    app::RemoteTrack track) const
{
    if (!services_.cache.cache_manager || track.id.empty()) {
        return track;
    }
    const auto cached = services_.cache.cache_manager->getText(
        ::lofibox::cache::CacheBucket::Metadata,
        app::remoteMediaCacheKey(profile, track.id));
    return cached ? preferCachedRemoteTrackFacts(std::move(track), app::parseRemoteTrackCache(*cached)) : track;
}

} // namespace lofibox::application
