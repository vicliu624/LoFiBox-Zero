// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/playback_runtime.h"

#include "playback/playback_state.h"

namespace lofibox::runtime {
namespace {

RuntimePlaybackStatus toRuntimeStatus(app::PlaybackStatus status) noexcept
{
    switch (status) {
    case app::PlaybackStatus::Empty: return RuntimePlaybackStatus::Empty;
    case app::PlaybackStatus::Paused: return RuntimePlaybackStatus::Paused;
    case app::PlaybackStatus::Playing: return RuntimePlaybackStatus::Playing;
    }
    return RuntimePlaybackStatus::Empty;
}

} // namespace

PlaybackRuntime::PlaybackRuntime(application::AppServiceRegistry services) noexcept
    : services_(services)
{
}

void PlaybackRuntime::update(double delta_seconds) const
{
    services_.playbackCommands().update(delta_seconds);
}

bool PlaybackRuntime::playFirstAvailable() const
{
    return services_.playbackCommands().playFirstAvailable();
}

bool PlaybackRuntime::startTrack(int track_id) const
{
    if (track_id <= 0) {
        return false;
    }
    return services_.playbackCommands().startTrack(track_id);
}

bool PlaybackRuntime::startRemoteStream(const RemotePlayResolvedStreamPayload& payload) const
{
    return services_.playbackCommands().startRemoteStream(payload.stream, payload.track, payload.source);
}

bool PlaybackRuntime::startRemoteLibraryTrack(const RemotePlayResolvedLibraryTrackPayload& payload) const
{
    return services_.playbackCommands().startRemoteLibraryTrack(
        payload.local_track_id,
        payload.stream,
        payload.profile,
        payload.track,
        payload.source,
        payload.cache_remote_facts);
}

void PlaybackRuntime::stop() const noexcept
{
    services_.playbackCommands().stop();
}

bool PlaybackRuntime::seek(double seconds) const
{
    return services_.playbackCommands().seek(seconds);
}

void PlaybackRuntime::pause() const noexcept
{
    services_.playbackCommands().pause();
}

void PlaybackRuntime::resume() const noexcept
{
    services_.playbackCommands().resume();
}

void PlaybackRuntime::togglePlayPause() const noexcept
{
    services_.playbackCommands().togglePlayPause();
}

PlaybackRuntimeSnapshot PlaybackRuntime::snapshot(std::uint64_t version) const
{
    const auto status = services_.playbackStatus().snapshot();
    const auto& session = services_.playbackStatus().session();
    PlaybackRuntimeSnapshot result{};
    result.status = toRuntimeStatus(status.status);
    result.current_track_id = status.current_track_id;
    result.title = status.title;
    result.source_label = status.source_label;
    result.elapsed_seconds = status.elapsed_seconds;
    result.duration_seconds = status.duration_seconds;
    result.audio_active = session.audio_active;
    result.shuffle_enabled = status.shuffle_enabled;
    result.repeat_all = status.repeat_all;
    result.repeat_one = status.repeat_one;
    result.version = version;
    return result;
}

} // namespace lofibox::runtime
