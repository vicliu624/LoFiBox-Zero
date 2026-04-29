// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/queue_runtime.h"

#include <utility>

namespace lofibox::runtime {

QueueRuntime::QueueRuntime(application::AppServiceRegistry services) noexcept
    : services_(services)
{
}

void QueueRuntime::setRemoteTrackStarter(RemoteTrackStarter starter)
{
    remote_track_starter_ = std::move(starter);
}

bool QueueRuntime::step(int delta) const
{
    if (delta == 0) {
        return false;
    }
    services_.queueCommands().step(delta, remote_track_starter_);
    return true;
}

void QueueRuntime::cycleMainMenuPlaybackMode() const
{
    services_.playbackCommands().cycleMainMenuPlaybackMode();
}

void QueueRuntime::toggleShuffle() const
{
    services_.playbackCommands().toggleShuffle();
}

void QueueRuntime::cycleRepeatMode() const noexcept
{
    services_.playbackCommands().cycleRepeatMode();
}

void QueueRuntime::setRepeatAll(bool enabled) const noexcept
{
    services_.playbackCommands().setRepeatAll(enabled);
}

void QueueRuntime::setRepeatOne(bool enabled) const noexcept
{
    services_.playbackCommands().setRepeatOne(enabled);
}

QueueRuntimeSnapshot QueueRuntime::snapshot(std::uint64_t version) const
{
    const auto queue = services_.playbackStatus().queueSnapshot();
    QueueRuntimeSnapshot result{};
    result.active_ids = queue.active_ids;
    result.active_index = queue.active_index;
    result.shuffle_enabled = queue.shuffle_enabled;
    result.repeat_all = queue.repeat_all;
    result.repeat_one = queue.repeat_one;
    result.version = version;
    return result;
}

} // namespace lofibox::runtime
