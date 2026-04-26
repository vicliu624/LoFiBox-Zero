// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/playback_controller.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <optional>
#include <random>
#include <utility>

namespace lofibox::app {

void PlaybackController::setServices(RuntimeServices services)
{
    services_ = std::move(services);
    enrichment_.setServices(services_);
    runtime_.setServices(&services_);
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
    enrichment_.applyPending(library_controller, session_);
    runtime_.tick(session_, queue_, library_controller, delta_seconds, [this, &library_controller](int index) {
        return playQueueIndex(library_controller, index);
    });
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
    runtime_.beginTrack(session_);
    refreshMetadata(library_controller, MetadataReadMode::LocalOnly);
    refreshArtwork(library_controller, ArtworkReadMode::LocalOnly);
    (void)runtime_.startBackend(track->path, session_);
    ++track->play_count;
    track->last_played = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    enrichment_.request(*track);
    return true;
}

void PlaybackController::stepQueue(LibraryController& library_controller, int delta)
{
    if (queue_.active_ids.empty()) {
        return;
    }
    runtime_.stepQueue(queue_, session_, delta, [this, &library_controller](int index) {
        return playQueueIndex(library_controller, index);
    });
}

void PlaybackController::pause() noexcept
{
    if (session_.status == PlaybackStatus::Playing) {
        runtime_.pause(session_);
    }
}

void PlaybackController::resume() noexcept
{
    if (session_.status == PlaybackStatus::Paused) {
        runtime_.resume(session_);
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

void PlaybackController::refreshArtwork(const LibraryController& library_controller, ArtworkReadMode mode)
{
    session_.current_artwork.reset();
    if (!session_.current_track_id) {
        return;
    }
    if (const auto* track = library_controller.findTrack(*session_.current_track_id)) {
        session_.current_artwork = services_.metadata.artwork_provider->read(track->path, mode);
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

    const auto metadata = services_.metadata.metadata_provider->read(track->path, mode);
    PlaybackEnrichmentCoordinator::applyMetadataToTrack(*track, metadata);
}

} // namespace lofibox::app
