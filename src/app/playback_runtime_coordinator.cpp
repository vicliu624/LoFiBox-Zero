// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/playback_runtime_coordinator.h"

namespace lofibox::app {

void PlaybackRuntimeCoordinator::setServices(RuntimeServices* services) noexcept
{
    audio_pipeline_.bind(services);
}

void PlaybackRuntimeCoordinator::beginTrack(PlaybackSession& session) const noexcept
{
    clock_.resetForTrack(session);
    session.current_lyrics = {};
    session.lyrics_lookup_pending = true;
    session.visualization_pending = false;
    session.visualization_frame = {};
}

bool PlaybackRuntimeCoordinator::startBackend(const std::filesystem::path& path, PlaybackSession& session)
{
    session.audio_active = audio_pipeline_.startFile(path, 0.0);
    return session.audio_active;
}

void PlaybackRuntimeCoordinator::pause(PlaybackSession& session) noexcept
{
    if (session.status == PlaybackStatus::Playing) {
        audio_pipeline_.pause();
        session.status = PlaybackStatus::Paused;
    }
}

void PlaybackRuntimeCoordinator::resume(PlaybackSession& session) noexcept
{
    if (session.status == PlaybackStatus::Paused) {
        audio_pipeline_.resume();
        session.status = PlaybackStatus::Playing;
    }
}

void PlaybackRuntimeCoordinator::stepQueue(const QueueState& queue, const PlaybackSession& session, int delta, const PlayIndexCallback& play_index) const
{
    const int index = completion_policy_.steppedIndex(queue, session, delta);
    if (index >= 0) {
        (void)play_index(index);
    }
}

bool PlaybackRuntimeCoordinator::advanceAfterFinish(const QueueState& queue, const PlaybackSession& session, const PlayIndexCallback& play_index) const
{
    const auto index = completion_policy_.nextIndexAfterFinish(queue, session);
    return index.has_value() && play_index(*index);
}

void PlaybackRuntimeCoordinator::tick(
    PlaybackSession& session,
    const QueueState& queue,
    const LibraryController& library_controller,
    double delta_seconds,
    const PlayIndexCallback& play_index)
{
    clock_.advance(session, delta_seconds);
    if (session.audio_active) {
        switch (audio_pipeline_.state()) {
        case AudioPlaybackState::Finished:
            if (advanceAfterFinish(queue, session, play_index)) {
                return;
            }
            session.audio_active = false;
            session.status = PlaybackStatus::Paused;
            if (session.current_track_id) {
                clock_.clampToTrackDuration(session, library_controller.findTrack(*session.current_track_id));
            }
            session.visualization_frame = {};
            return;
        case AudioPlaybackState::Failed:
            session.audio_active = false;
            session.status = PlaybackStatus::Paused;
            session.visualization_frame = {};
            return;
        case AudioPlaybackState::Idle:
        case AudioPlaybackState::Starting:
        case AudioPlaybackState::Playing:
        case AudioPlaybackState::Paused:
            break;
        }
    }
    session.visualization_frame = audio_pipeline_.visualizationFrame();
}

} // namespace lofibox::app
