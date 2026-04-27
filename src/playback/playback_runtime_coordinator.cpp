// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/playback_runtime_coordinator.h"

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

bool PlaybackRuntimeCoordinator::startBackendUri(const std::string& uri, PlaybackSession& session)
{
    session.audio_active = audio_pipeline_.startUri(uri, 0.0);
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
    const auto duration = session.current_track_id
        ? std::chrono::milliseconds{static_cast<int>((library_controller.findTrack(*session.current_track_id) ? library_controller.findTrack(*session.current_track_id)->duration_seconds : 0) * 1000)}
        : std::chrono::milliseconds{0};
    const PlaybackStabilitySample stability_sample{
        std::chrono::milliseconds{static_cast<int>(session.elapsed_seconds * 1000.0)},
        duration,
        session.audio_active && session.elapsed_seconds < 0.25,
        false,
        false};
    if (stability_policy_.shouldPrepareNext(stability_sample, transition_policy_)) {
        session.visualization_pending = true;
    }
    if (session.audio_active) {
        switch (audio_pipeline_.state()) {
        case AudioPlaybackState::Finished:
            if (stability_policy_.suppressStartOrEndJitter(PlaybackStabilitySample{
                    std::chrono::milliseconds{static_cast<int>(session.elapsed_seconds * 1000.0)},
                    duration,
                    false,
                    true,
                    false})) {
                return;
            }
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

