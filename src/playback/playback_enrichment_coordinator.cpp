// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/playback_enrichment_coordinator.h"

#include <filesystem>
#include <optional>
#include <utility>

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

void PlaybackEnrichmentCoordinator::applyPending(LibraryController& library_controller, PlaybackSession& session)
{
    std::vector<PlaybackEnrichmentResult> results{};
    {
        std::lock_guard lock(mutex_);
        results.swap(pending_results_);
    }

    for (auto& result : results) {
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

