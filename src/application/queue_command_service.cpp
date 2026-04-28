// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/queue_command_service.h"

namespace lofibox::application {

QueueCommandService::QueueCommandService(PlaybackCommandService playback) noexcept
    : playback_(playback)
{
}

void QueueCommandService::step(int delta, const PlaybackCommandService::RemoteTrackStarter& remote_starter) const
{
    playback_.stepQueue(delta, remote_starter);
}

void QueueCommandService::prepareForTrack(int track_id) const
{
    playback_.prepareQueueForTrack(track_id);
}

} // namespace lofibox::application
