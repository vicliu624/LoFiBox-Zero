// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/playback_enrichment_coordinator.h"

#include <filesystem>
#include <optional>
#include <utility>

#include "app/remote_media_contract.h"
#include "cache/cache_manager.h"
#include "metadata/metadata_merge_policy.h"

namespace lofibox::app {
namespace {

TrackMetadata mergeLyricsSeed(TrackMetadata seed_metadata, const TrackMetadata& metadata)
{
    if (metadata.title) seed_metadata.title = metadata.title;
    if (metadata.artist) seed_metadata.artist = metadata.artist;
    if (metadata.album) seed_metadata.album = metadata.album;
    if (metadata.genre) seed_metadata.genre = metadata.genre;
    if (metadata.composer) seed_metadata.composer = metadata.composer;
    if (metadata.duration_seconds) seed_metadata.duration_seconds = metadata.duration_seconds;
    return seed_metadata;
}

} // namespace

PlaybackEnrichmentCoordinator::~PlaybackEnrichmentCoordinator()
{
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void PlaybackEnrichmentCoordinator::setServices(RuntimeServices services)
{
    services_ = std::move(services);
}

void PlaybackEnrichmentCoordinator::request(const TrackRecord& track)
{
    const auto services_snapshot = services_;
    const int track_id = track.id;
    const std::filesystem::path path = track.path;
    const TrackMetadata seed_metadata = metadataFromTrack(track);
    const std::uint64_t generation = ++generation_;

    threads_.emplace_back([this, services_snapshot, track_id, path, seed_metadata, generation]() {
        PlaybackEnrichmentResult result{};
        result.track_id = track_id;
        result.path = path;
        result.generation = generation;
        result.metadata = services_snapshot.metadata.metadata_provider->read(path, MetadataReadMode::AllowOnline);

        const auto lyrics_metadata = mergeLyricsSeed(seed_metadata, result.metadata);
        result.artwork = services_snapshot.metadata.artwork_provider->read(path);
        result.lyrics = services_snapshot.metadata.lyrics_provider->fetch(path, lyrics_metadata);

        std::lock_guard lock(mutex_);
        pending_results_.push_back(std::move(result));
    });
}

void PlaybackEnrichmentCoordinator::requestRemote(
    const RemoteServerProfile& profile,
    const RemoteTrack& remote_track,
    int track_id,
    std::string stream_uri,
    bool cache_remote_facts)
{
    if (!remoteTrackNeedsLocalMetadataGovernance(remote_track)) {
        return;
    }

    const auto services_snapshot = services_;
    const std::filesystem::path lookup_path = remoteTrackSyntheticLookupPath(profile, remote_track);
    const TrackMetadata seed_metadata = remoteTrackMetadataSeed(remote_track);
    const std::string stable_cache_key = remoteMediaCacheKey(profile, remote_track.id);
    const std::uint64_t generation = ++generation_;

    threads_.emplace_back([this, services_snapshot, profile, remote_track, track_id, stream_uri = std::move(stream_uri), lookup_path, seed_metadata, stable_cache_key, cache_remote_facts, generation]() {
        PlaybackEnrichmentResult result{};
        result.track_id = track_id;
        result.path = lookup_path;
        result.generation = generation;
        result.remote = true;
        result.remote_profile = profile;
        result.remote_track = remote_track;
        result.cache_remote_facts = cache_remote_facts;

        if (services_snapshot.metadata.track_identity_provider
            && services_snapshot.metadata.track_identity_provider->available()) {
            const auto identity = services_snapshot.metadata.track_identity_provider->resolveRemoteStream(
                stable_cache_key,
                stream_uri,
                lookup_path,
                seed_metadata);
            if (!identity.fingerprint.empty()) {
                result.identity.fingerprint = identity.fingerprint;
            }
            if (identity.found) {
                result.identity = identity;
                result.metadata = identity.metadata;
            }
        }

        if (services_snapshot.metadata.lyrics_provider
            && services_snapshot.metadata.lyrics_provider->available()) {
            const auto lyrics_metadata = mergeLyricsSeed(seed_metadata, result.metadata);
            result.lyrics = services_snapshot.metadata.lyrics_provider->fetch(lookup_path, lyrics_metadata);
        }

        if (services_snapshot.metadata.artwork_provider
            && services_snapshot.metadata.artwork_provider->available()) {
            result.artwork = services_snapshot.metadata.artwork_provider->readRemoteIdentity(
                stable_cache_key,
                lookup_path,
                ArtworkReadMode::AllowOnline);
        }

        std::lock_guard lock(mutex_);
        pending_results_.push_back(std::move(result));
    });
}

void PlaybackEnrichmentCoordinator::applyPending(LibraryController& library_controller, PlaybackSession& session)
{
    std::vector<PlaybackEnrichmentResult> results{};
    {
        std::lock_guard lock(mutex_);
        results.swap(pending_results_);
    }

    for (auto& result : results) {
        if (result.remote) {
            auto* track = library_controller.findMutableTrack(result.track_id);
            if (!track
                || !track->remote
                || track->remote_profile_id != result.remote_profile.id
                || track->remote_track_id != result.remote_track.id) {
                continue;
            }

            auto governed_track = mergeRemoteGovernedFacts(result.remote_track, result.metadata, result.lyrics);
            if (!result.identity.fingerprint.empty()) {
                governed_track.fingerprint = result.identity.fingerprint;
            }
            if (governed_track.source_id.empty()) {
                governed_track.source_id = result.remote_profile.id;
            }
            if (governed_track.source_label.empty()) {
                governed_track.source_label = result.remote_profile.name;
            }
            (void)library_controller.applyRemoteTrackFacts(result.remote_profile, governed_track);

            if (result.cache_remote_facts && services_.cache.cache_manager && remoteTrackHasLocalCacheableFacts(governed_track)) {
                (void)services_.cache.cache_manager->putText(
                    ::lofibox::cache::CacheBucket::Metadata,
                    remoteMediaCacheKey(result.remote_profile, governed_track.id),
                    serializeRemoteTrackCache(governed_track));
                if (!governed_track.lyrics_plain.empty() || !governed_track.lyrics_synced.empty()) {
                    (void)services_.cache.cache_manager->putText(
                        ::lofibox::cache::CacheBucket::Lyrics,
                        remoteMediaCacheKey(result.remote_profile, governed_track.id),
                        serializeRemoteTrackCache(governed_track));
                }
            }

            if (!session.current_track_id || *session.current_track_id != result.track_id) {
                continue;
            }
            if (result.generation != generation_) {
                continue;
            }
            if (!governed_track.lyrics_plain.empty()) {
                session.current_lyrics.plain = governed_track.lyrics_plain;
            }
            if (!governed_track.lyrics_synced.empty()) {
                session.current_lyrics.synced = governed_track.lyrics_synced;
            }
            if (!governed_track.lyrics_source.empty()) {
                session.current_lyrics.source = governed_track.lyrics_source;
            }
            if (result.artwork) {
                session.current_artwork = std::move(result.artwork);
            }
            session.lyrics_lookup_pending = false;
            continue;
        }

        auto* track = library_controller.findMutableTrack(result.track_id);
        if (!track || track->path != result.path) {
            continue;
        }

        applyMetadataToTrack(*track, result.metadata);

        if (!session.current_track_id || *session.current_track_id != result.track_id) {
            continue;
        }
        if (result.generation != generation_) {
            continue;
        }

        if (result.artwork) {
            session.current_artwork = std::move(result.artwork);
        }
        session.current_lyrics = std::move(result.lyrics);
        session.lyrics_lookup_pending = false;
    }
}

TrackMetadata PlaybackEnrichmentCoordinator::metadataFromTrack(const TrackRecord& track)
{
    TrackMetadata metadata{};
    metadata.title = track.title;
    metadata.artist = track.artist;
    metadata.album = track.album;
    metadata.genre = track.genre;
    metadata.composer = track.composer;
    metadata.duration_seconds = track.duration_seconds;
    return metadata;
}

void PlaybackEnrichmentCoordinator::applyMetadataToTrack(TrackRecord& track, const TrackMetadata& metadata)
{
    const ::lofibox::metadata::MetadataMergePolicy merge_policy{};
    merge_policy.mergeIntoTrack(track, metadata);
}

} // namespace lofibox::app

