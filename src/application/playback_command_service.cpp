// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/playback_command_service.h"

#include <utility>

#include "application/library_mutation_service.h"

namespace lofibox::application {

PlaybackCommandService::PlaybackCommandService(app::PlaybackController& playback, app::LibraryController& library) noexcept
    : playback_(playback),
      library_(library)
{
    playback_.setPlaybackStartedRecorder([&library](app::TrackRecord& track) {
        LibraryMutationService{library}.recordPlaybackStarted(track);
    });
}

const app::PlaybackSession& PlaybackCommandService::session() const noexcept
{
    return playback_.session();
}

void PlaybackCommandService::update(double delta_seconds, const RemoteTrackStarter& remote_starter) const
{
    if (remote_starter) {
        playback_.update(delta_seconds, library_, remote_starter);
        return;
    }
    playback_.update(delta_seconds, library_);
}

void PlaybackCommandService::setDspProfile(::lofibox::audio::dsp::DspChainProfile profile) const
{
    playback_.setDspProfile(std::move(profile));
}

bool PlaybackCommandService::playFirstAvailable(const RemoteTrackStarter& remote_starter) const
{
    const auto& current = playback_.session();
    if (current.status == app::PlaybackStatus::Paused && (current.current_track_id || !current.current_stream_title.empty())) {
        playback_.resume();
        return true;
    }
    if (current.status == app::PlaybackStatus::Playing) {
        return true;
    }
    if (current.current_track_id || !current.current_stream_title.empty()) {
        playback_.resume();
        return true;
    }

    const auto ids = library_.allSongIdsSorted();
    if (ids.empty()) {
        return false;
    }
    library_.setSongsContextAll();
    return startTrack(ids.front(), remote_starter);
}

bool PlaybackCommandService::startTrack(int track_id, const RemoteTrackStarter& remote_starter) const
{
    const auto* track = library_.findTrack(track_id);
    if (track == nullptr) {
        return false;
    }
    if (!track->remote) {
        return playback_.startTrack(library_, track_id);
    }
    playback_.prepareQueueForTrack(library_, track_id);
    return remote_starter ? remote_starter(track_id) : false;
}

void PlaybackCommandService::prepareQueueForTrack(int track_id) const
{
    playback_.prepareQueueForTrack(library_, track_id);
}

void PlaybackCommandService::stepQueue(int delta, const RemoteTrackStarter& remote_starter) const
{
    if (remote_starter) {
        playback_.stepQueue(library_, delta, remote_starter);
        return;
    }
    playback_.stepQueue(library_, delta);
}

bool PlaybackCommandService::jumpQueue(int queue_index, const RemoteTrackStarter& remote_starter) const
{
    if (remote_starter) {
        return playback_.jumpQueue(library_, queue_index, remote_starter);
    }
    return playback_.jumpQueue(library_, queue_index);
}

void PlaybackCommandService::clearQueue() const noexcept
{
    playback_.clearQueue();
}

void PlaybackCommandService::pause() const noexcept
{
    playback_.pause();
}

void PlaybackCommandService::resume() const noexcept
{
    playback_.resume();
}

void PlaybackCommandService::stop() const noexcept
{
    playback_.stop();
}

bool PlaybackCommandService::seek(double seconds) const
{
    return playback_.seek(library_, seconds);
}

void PlaybackCommandService::togglePlayPause() const noexcept
{
    playback_.togglePlayPause();
}

void PlaybackCommandService::setShuffleEnabled(bool enabled) const
{
    playback_.setShuffleEnabled(enabled);
}

void PlaybackCommandService::toggleShuffle() const
{
    playback_.toggleShuffle();
}

void PlaybackCommandService::setRepeatAll(bool enabled) const noexcept
{
    playback_.setRepeatAll(enabled);
}

void PlaybackCommandService::setRepeatOne(bool enabled) const noexcept
{
    playback_.setRepeatOne(enabled);
}

void PlaybackCommandService::cycleRepeatMode() const noexcept
{
    playback_.cycleRepeatMode();
}

void PlaybackCommandService::cycleMainMenuPlaybackMode() const
{
    const auto& current = playback_.session();
    if (current.shuffle_enabled) {
        playback_.setShuffleEnabled(false);
        playback_.setRepeatAll(true);
        return;
    }
    if (current.repeat_all) {
        playback_.setRepeatAll(false);
        playback_.setRepeatOne(true);
        return;
    }
    if (current.repeat_one) {
        playback_.setRepeatOne(false);
        playback_.setRepeatAll(false);
        return;
    }
    playback_.setRepeatOne(false);
    playback_.setRepeatAll(false);
    playback_.setShuffleEnabled(true);
}

bool PlaybackCommandService::startRemoteStream(const app::ResolvedRemoteStream& stream, const app::RemoteTrack& track, const std::string& source) const
{
    return playback_.startRemoteStream(stream, track, source);
}

bool PlaybackCommandService::startRemoteLibraryTrack(
    int local_track_id,
    const app::ResolvedRemoteStream& stream,
    const app::RemoteServerProfile& profile,
    const app::RemoteTrack& remote_track,
    const std::string& source,
    bool cache_remote_facts) const
{
    auto* track = library_.findMutableTrack(local_track_id);
    return track != nullptr && startRemoteLibraryTrack(stream, *track, profile, remote_track, source, cache_remote_facts);
}

bool PlaybackCommandService::startRemoteLibraryTrack(
    const app::ResolvedRemoteStream& stream,
    app::TrackRecord& track,
    const app::RemoteServerProfile& profile,
    const app::RemoteTrack& remote_track,
    const std::string& source,
    bool cache_remote_facts) const
{
    return playback_.startRemoteLibraryTrack(stream, track, profile, remote_track, source, cache_remote_facts);
}

} // namespace lofibox::application
