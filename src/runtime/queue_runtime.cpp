// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/queue_runtime.h"

#include <string>
#include <unordered_map>
#include <utility>

namespace lofibox::runtime {

QueueRuntime::QueueRuntime(application::AppServiceRegistry services) noexcept
    : services_(services)
{
}

bool QueueRuntime::step(int delta) const
{
    if (delta == 0) {
        return false;
    }
    return services_.queueCommands().step(delta);
}

bool QueueRuntime::jump(int queue_index) const
{
    if (queue_index < 0) {
        return false;
    }
    return services_.queueCommands().jump(queue_index);
}

void QueueRuntime::clear() const noexcept
{
    services_.queueCommands().clear();
}

void QueueRuntime::prepareForTrack(int track_id) const
{
    services_.queueCommands().prepareForTrack(track_id);
}

void QueueRuntime::cycleMainMenuPlaybackMode() const
{
    services_.playbackCommands().cycleMainMenuPlaybackMode();
}

void QueueRuntime::toggleShuffle() const
{
    services_.playbackCommands().toggleShuffle();
}

void QueueRuntime::setShuffleEnabled(bool enabled) const noexcept
{
    services_.playbackCommands().setShuffleEnabled(enabled);
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
    const auto library = services_.libraryQueries();
    const auto lock = library.lockState();
    auto queue = services_.playbackStatus().queueSnapshot();
    QueueRuntimeSnapshot result{};
    result.active_ids = std::move(queue.active_ids);
    result.active_index = queue.active_index;
    result.selected_index = queue.active_index;
    result.shuffle_enabled = queue.shuffle_enabled;
    result.repeat_all = queue.repeat_all;
    result.repeat_one = queue.repeat_one;
    result.version = version;
    if (result.active_ids.empty()) return result;

    // A full snapshot is shared by the GUI, TUI and external clients. Avoid
    // one linear library search for every queue entry on each refresh.
    std::unordered_map<int, const app::TrackRecord*> tracks;
    tracks.reserve(library.model().tracks.size());
    for (const auto& track : library.model().tracks) tracks.emplace(track.id, &track);
    result.visible_items.reserve(result.active_ids.size());

    int queue_index = 0;
    for (const int track_id : result.active_ids) {
        RuntimeQueueItem item{};
        item.track_id = track_id;
        item.queue_index = queue_index;
        item.active = queue_index == result.active_index;
        if (const auto found = tracks.find(track_id); found != tracks.end()) {
            const auto* track = found->second;
            item.title = track->title;
            item.artist = track->artist;
            item.album = track->album;
            item.source_label = track->remote && !track->source_label.empty() ? track->source_label : "LOCAL";
            item.duration_seconds = track->duration_seconds;
        } else {
            item.title = "Track " + std::to_string(track_id);
            item.source_label = "UNKNOWN";
            item.playable = false;
        }
        result.visible_items.push_back(std::move(item));
        ++queue_index;
    }
    return result;
}

} // namespace lofibox::runtime
