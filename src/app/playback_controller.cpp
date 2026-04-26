// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/playback_controller.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <utility>

namespace lofibox::app {
namespace {

std::string metadataOrCurrent(const std::optional<std::string>& metadata_value, const std::string& current)
{
    if (metadata_value && !metadata_value->empty()) {
        return *metadata_value;
    }
    return current;
}

} // namespace

PlaybackController::~PlaybackController()
{
    for (auto& thread : enrichment_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void PlaybackController::setServices(RuntimeServices services)
{
    services_ = std::move(services);
}

const PlaybackSession& PlaybackController::session() const noexcept
{
    return session_;
}

PlaybackSession& PlaybackController::mutableSession() noexcept
{
    return session_;
}

void PlaybackController::update(double delta_seconds, LibraryController& library_controller)
{
    applyPendingEnrichments(library_controller);
    if (session_.status == PlaybackStatus::Playing) {
        session_.elapsed_seconds += std::max(0.0, delta_seconds);
    }
    synchronizeBackendState(library_controller);
    updateVisualizationFrame();
}

bool PlaybackController::startTrack(LibraryController& library_controller, int track_id)
{
    rebuildQueueForCurrentSongs(library_controller, track_id);
    return playQueueIndex(library_controller, queue_.active_index);
}

bool PlaybackController::playQueueIndex(LibraryController& library_controller, int queue_index)
{
    if (queue_index < 0 || queue_index >= static_cast<int>(queue_.active_ids.size())) {
        return false;
    }

    auto* track = library_controller.findMutableTrack(queue_.active_ids[static_cast<std::size_t>(queue_index)]);
    if (!track) {
        return false;
    }

    queue_.active_index = queue_index;
    session_.current_track_id = track->id;
    session_.status = PlaybackStatus::Playing;
    session_.elapsed_seconds = 0.0;
    session_.current_lyrics = {};
    session_.lyrics_lookup_pending = true;
    session_.visualization_pending = false;
    session_.visualization_frame = {};
    refreshMetadata(library_controller, MetadataReadMode::LocalOnly);
    refreshArtwork(library_controller, ArtworkReadMode::LocalOnly);
    session_.audio_active = services_.audio_playback_backend->playFile(track->path, 0.0);
    ++track->play_count;
    track->last_played = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    requestEnrichment(*track);
    return true;
}

void PlaybackController::stepQueue(LibraryController& library_controller, int delta)
{
    if (queue_.active_ids.empty()) {
        return;
    }
    int next = queue_.active_index + delta;
    if (next < 0) {
        next = session_.repeat_all ? static_cast<int>(queue_.active_ids.size()) - 1 : 0;
    }
    if (next >= static_cast<int>(queue_.active_ids.size())) {
        next = session_.repeat_all ? 0 : static_cast<int>(queue_.active_ids.size()) - 1;
    }
    (void)playQueueIndex(library_controller, std::clamp(next, 0, static_cast<int>(queue_.active_ids.size()) - 1));
}

void PlaybackController::pause() noexcept
{
    if (session_.status == PlaybackStatus::Playing) {
        if (services_.audio_playback_backend) {
            services_.audio_playback_backend->pause();
        }
        session_.status = PlaybackStatus::Paused;
    }
}

void PlaybackController::resume() noexcept
{
    if (session_.status == PlaybackStatus::Paused) {
        if (services_.audio_playback_backend) {
            services_.audio_playback_backend->resume();
        }
        session_.status = PlaybackStatus::Playing;
    }
}

void PlaybackController::togglePlayPause() noexcept
{
    if (session_.status == PlaybackStatus::Playing) {
        pause();
    } else if (session_.status == PlaybackStatus::Paused) {
        resume();
    }
}

void PlaybackController::setShuffleEnabled(bool enabled)
{
    if (session_.shuffle_enabled == enabled) {
        return;
    }
    session_.shuffle_enabled = enabled;
    rebuildActiveQueueForMode(session_.current_track_id);
}

void PlaybackController::toggleShuffle()
{
    setShuffleEnabled(!session_.shuffle_enabled);
}

void PlaybackController::setRepeatAll(bool enabled) noexcept
{
    session_.repeat_all = enabled;
    if (enabled) {
        session_.repeat_one = false;
    }
}

void PlaybackController::setRepeatOne(bool enabled) noexcept
{
    session_.repeat_one = enabled;
    if (enabled) {
        session_.repeat_all = false;
    }
}

void PlaybackController::cycleRepeatMode() noexcept
{
    if (!session_.repeat_all && !session_.repeat_one) {
        setRepeatAll(true);
    } else if (session_.repeat_all) {
        setRepeatOne(true);
    } else {
        session_.repeat_all = false;
        session_.repeat_one = false;
    }
}

void PlaybackController::rebuildQueueForCurrentSongs(const LibraryController& library_controller, int selected_track_id)
{
    queue_.base_ids = library_controller.trackIdsForCurrentSongs();
    if (queue_.base_ids.empty()) {
        queue_.base_ids = library_controller.allSongIdsSorted();
    }
    rebuildActiveQueueForMode(selected_track_id);
}

void PlaybackController::rebuildActiveQueueForMode(std::optional<int> selected_track_id)
{
    queue_.active_ids = queue_.base_ids;
    if (session_.shuffle_enabled && queue_.active_ids.size() > 1) {
        static std::mt19937 rng{std::random_device{}()};
        std::shuffle(queue_.active_ids.begin(), queue_.active_ids.end(), rng);
    }
    const auto it = selected_track_id
        ? std::find(queue_.active_ids.begin(), queue_.active_ids.end(), *selected_track_id)
        : queue_.active_ids.end();
    queue_.active_index = it == queue_.active_ids.end() ? 0 : static_cast<int>(std::distance(queue_.active_ids.begin(), it));
}

bool PlaybackController::advanceAfterFinish(LibraryController& library_controller)
{
    if (queue_.active_ids.empty()) {
        return false;
    }
    if (session_.repeat_one) {
        return playQueueIndex(library_controller, std::clamp(queue_.active_index, 0, static_cast<int>(queue_.active_ids.size()) - 1));
    }
    if (queue_.active_index + 1 < static_cast<int>(queue_.active_ids.size())) {
        return playQueueIndex(library_controller, queue_.active_index + 1);
    }
    if (session_.repeat_all) {
        return playQueueIndex(library_controller, 0);
    }
    return false;
}

void PlaybackController::refreshArtwork(const LibraryController& library_controller, ArtworkReadMode mode)
{
    session_.current_artwork.reset();
    if (!session_.current_track_id) {
        return;
    }
    if (const auto* track = library_controller.findTrack(*session_.current_track_id)) {
        session_.current_artwork = services_.artwork_provider->read(track->path, mode);
    }
}

void PlaybackController::refreshMetadata(LibraryController& library_controller, MetadataReadMode mode)
{
    if (!session_.current_track_id) {
        return;
    }
    auto* track = library_controller.findMutableTrack(*session_.current_track_id);
    if (!track) {
        return;
    }

    const auto metadata = services_.metadata_provider->read(track->path, mode);
    applyMetadataToTrack(*track, metadata);
}

void PlaybackController::requestEnrichment(const TrackRecord& track)
{
    const auto services_snapshot = services_;
    const int track_id = track.id;
    const std::filesystem::path path = track.path;
    const TrackMetadata seed_metadata = metadataFromTrack(track);
    const std::uint64_t generation = ++enrichment_generation_;

    enrichment_threads_.emplace_back([this, services_snapshot, track_id, path, seed_metadata, generation]() {
        PlaybackEnrichmentResult result{};
        result.track_id = track_id;
        result.path = path;
        result.generation = generation;
        result.metadata = services_snapshot.metadata_provider->read(path, MetadataReadMode::AllowOnline);

        TrackMetadata lyrics_metadata = seed_metadata;
        if (result.metadata.title) lyrics_metadata.title = result.metadata.title;
        if (result.metadata.artist) lyrics_metadata.artist = result.metadata.artist;
        if (result.metadata.album) lyrics_metadata.album = result.metadata.album;
        if (result.metadata.genre) lyrics_metadata.genre = result.metadata.genre;
        if (result.metadata.composer) lyrics_metadata.composer = result.metadata.composer;
        if (result.metadata.duration_seconds) lyrics_metadata.duration_seconds = result.metadata.duration_seconds;

        result.artwork = services_snapshot.artwork_provider->read(path);
        result.lyrics = services_snapshot.lyrics_provider->fetch(path, lyrics_metadata);

        std::lock_guard lock(enrichment_mutex_);
        pending_enrichments_.push_back(std::move(result));
    });
}

void PlaybackController::applyPendingEnrichments(LibraryController& library_controller)
{
    std::vector<PlaybackEnrichmentResult> results{};
    {
        std::lock_guard lock(enrichment_mutex_);
        results.swap(pending_enrichments_);
    }

    for (auto& result : results) {
        auto* track = library_controller.findMutableTrack(result.track_id);
        if (!track || track->path != result.path) {
            continue;
        }

        applyMetadataToTrack(*track, result.metadata);

        if (!session_.current_track_id || *session_.current_track_id != result.track_id) {
            continue;
        }
        if (result.generation != enrichment_generation_) {
            continue;
        }

        if (result.artwork) {
            session_.current_artwork = std::move(result.artwork);
        }
        session_.current_lyrics = std::move(result.lyrics);
        session_.lyrics_lookup_pending = false;
    }
}

TrackMetadata PlaybackController::metadataFromTrack(const TrackRecord& track)
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

void PlaybackController::applyMetadataToTrack(TrackRecord& track, const TrackMetadata& metadata)
{
    if (metadata.title) track.title = metadataOrCurrent(metadata.title, track.title);
    if (metadata.artist) track.artist = metadataOrCurrent(metadata.artist, track.artist);
    if (metadata.album) track.album = metadataOrCurrent(metadata.album, track.album);
    if (metadata.genre) track.genre = metadataOrCurrent(metadata.genre, track.genre);
    if (metadata.composer) track.composer = metadataOrCurrent(metadata.composer, track.composer);
    if (metadata.duration_seconds) track.duration_seconds = *metadata.duration_seconds;
}

void PlaybackController::synchronizeBackendState(LibraryController& library_controller)
{
    if (!session_.audio_active || !services_.audio_playback_backend) {
        return;
    }

    switch (services_.audio_playback_backend->state()) {
    case AudioPlaybackState::Finished:
        if (advanceAfterFinish(library_controller)) {
            return;
        }
        session_.audio_active = false;
        session_.status = PlaybackStatus::Paused;
        if (session_.current_track_id) {
            if (const auto* track = library_controller.findTrack(*session_.current_track_id)) {
                session_.elapsed_seconds = std::min(session_.elapsed_seconds, static_cast<double>(std::max(0, track->duration_seconds)));
            }
        }
        session_.visualization_frame = {};
        return;
    case AudioPlaybackState::Failed:
        session_.audio_active = false;
        session_.status = PlaybackStatus::Paused;
        session_.visualization_frame = {};
        return;
    case AudioPlaybackState::Idle:
    case AudioPlaybackState::Starting:
    case AudioPlaybackState::Playing:
    case AudioPlaybackState::Paused:
        return;
    }
}

void PlaybackController::updateVisualizationFrame()
{
    if (!services_.audio_playback_backend) {
        session_.visualization_frame = {};
        return;
    }
    session_.visualization_frame = services_.audio_playback_backend->visualizationFrame();
}

} // namespace lofibox::app
