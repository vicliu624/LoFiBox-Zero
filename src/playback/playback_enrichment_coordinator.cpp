// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/playback_enrichment_coordinator.h"

#include <filesystem>
#include <optional>
#include <utility>

#include "app/remote_media_contract.h"
#include "cache/cache_manager.h"
#include "metadata/metadata_merge_policy.h"
#include "core/operation_cancellation.h"

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
    {
        const std::lock_guard lock{mutex_};
        stopping_ = true;
        pending_job_ = {};
        active_cancellation_.request_stop();
    }
    work_ready_.notify_one();
    if (worker_.joinable()) worker_.join();
}

void PlaybackEnrichmentCoordinator::enqueue(std::function<void(std::stop_token)> job)
{
    const std::lock_guard lock{mutex_};
    active_cancellation_.request_stop();
    pending_job_ = std::move(job); // Keep only the newest requested track.
    if (!worker_.joinable()) worker_ = std::thread([this] { runWorker(); });
    work_ready_.notify_one();
}

void PlaybackEnrichmentCoordinator::runWorker()
{
    while (true) {
        std::function<void(std::stop_token)> job;
        std::stop_token token;
        {
            std::unique_lock lock{mutex_};
            work_ready_.wait(lock, [this] { return stopping_ || static_cast<bool>(pending_job_); });
            if (stopping_) return;
            job = std::move(pending_job_);
            pending_job_ = {};
            active_cancellation_ = std::stop_source{};
            token = active_cancellation_.get_token();
        }
        const core::OperationCancellationScope cancellation{token};
        try { job(token); }
        catch (...) { /* A failed background task must not terminate the worker. */ }
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

    enqueue([this, services_snapshot, track_id, path, seed_metadata, generation](std::stop_token token) {
        const auto publish = [this, token](PlaybackEnrichmentResult result) {
            if (token.stop_requested()) return;
            const std::lock_guard lock{mutex_};
            if (token.stop_requested()) return;
            if (pending_results_.size() >= 8) pending_results_.erase(pending_results_.begin());
            pending_results_.push_back(std::move(result));
        };
        PlaybackEnrichmentResult local{};
        local.track_id = track_id;
        local.path = path;
        local.generation = generation;
        local.complete = false;
        try {
            local.metadata = services_snapshot.metadata.metadata_provider->read(path, MetadataReadMode::LocalOnly);
            if (token.stop_requested()) return;
            local.artwork = services_snapshot.metadata.artwork_provider->read(path, ArtworkReadMode::LocalOnly);
        } catch (...) { /* Publish available local facts even when a provider fails. */ }
        if (token.stop_requested()) return;
        const auto local_seed = mergeLyricsSeed(seed_metadata, local.metadata);
        publish(std::move(local));

        PlaybackEnrichmentResult result{};
        result.track_id = track_id;
        result.path = path;
        result.generation = generation;
        try {
            result.metadata = services_snapshot.metadata.metadata_provider->read(path, MetadataReadMode::AllowOnline);
            if (token.stop_requested()) return;
            const auto lyrics_metadata = mergeLyricsSeed(local_seed, result.metadata);
            result.artwork = services_snapshot.metadata.artwork_provider->read(path);
            if (token.stop_requested()) return;
            result.lyrics = services_snapshot.metadata.lyrics_provider->fetch(path, lyrics_metadata);
        } catch (...) { /* Keep available facts; an enrichment failure must not stop playback. */ }
        if (token.stop_requested()) return;

        publish(std::move(result));
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

    enqueue([this, services_snapshot, profile, remote_track, track_id, stream_uri = std::move(stream_uri), lookup_path, seed_metadata, stable_cache_key, cache_remote_facts, generation](std::stop_token token) {
        PlaybackEnrichmentResult result{};
        result.track_id = track_id;
        result.path = lookup_path;
        result.generation = generation;
        result.remote = true;
        result.remote_profile = profile;
        result.remote_track = remote_track;
        result.cache_remote_facts = cache_remote_facts;

        try {
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

        if (token.stop_requested()) return;
        if (services_snapshot.metadata.lyrics_provider) {
            const auto lyrics_metadata = mergeLyricsSeed(seed_metadata, result.metadata);
            result.lyrics = services_snapshot.metadata.lyrics_provider->fetchRemoteIdentity(
                stable_cache_key,
                lookup_path,
                lyrics_metadata,
                result.identity);
        }

        if (token.stop_requested()) return;
        if (services_snapshot.metadata.artwork_provider
            && services_snapshot.metadata.artwork_provider->available()) {
            result.artwork = services_snapshot.metadata.artwork_provider->readRemoteIdentity(
                stable_cache_key,
                lookup_path,
                ArtworkReadMode::AllowOnline,
                remote_track.artwork_url);
        }

        } catch (...) { /* Preserve partial facts on a provider failure. */ }
        if (token.stop_requested()) return;
        std::lock_guard lock(mutex_);
        if (token.stop_requested()) return;
        if (pending_results_.size() >= 8) pending_results_.erase(pending_results_.begin());
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

            auto governed_track = mergeRemoteGovernedFacts(result.remote_track, result.metadata, result.lyrics, result.identity);
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
        if (result.complete) {
            session.current_lyrics = std::move(result.lyrics);
            session.lyrics_lookup_pending = false;
        }
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
    if (track.pending_local_title && metadata.title && !metadata.title->empty()) {
        track.title = *metadata.title;
        track.pending_local_title = false;
    }
    const ::lofibox::metadata::MetadataMergePolicy merge_policy{};
    merge_policy.mergeIntoTrack(track, metadata);
}

} // namespace lofibox::app

