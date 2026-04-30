// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/queue_command_service.h"

namespace lofibox::application {

QueueCommandService::QueueCommandService(PlaybackCommandService playback) noexcept
    : playback_(playback)
{
}

bool QueueCommandService::step(int delta, const PlaybackCommandService::RemoteTrackStarter& remote_starter) const
{
    return playback_.stepQueue(delta, remote_starter);
}

bool QueueCommandService::jump(int queue_index, const PlaybackCommandService::RemoteTrackStarter& remote_starter) const
{
    return playback_.jumpQueue(queue_index, remote_starter);
}

void QueueCommandService::clear() const noexcept
{
    playback_.clearQueue();
}

void QueueCommandService::prepareForTrack(int track_id) const
{
    playback_.prepareQueueForTrack(track_id);
}

} // namespace lofibox::application
