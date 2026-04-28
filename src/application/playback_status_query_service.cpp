// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/playback_status_query_service.h"

#include "app/library_controller.h"
#include "playback/playback_controller.h"

namespace lofibox::application {

PlaybackStatusQueryService::PlaybackStatusQueryService(const app::PlaybackController& playback, const app::LibraryController& library) noexcept
    : playback_(playback),
      library_(library)
{
}

const app::PlaybackSession& PlaybackStatusQueryService::session() const noexcept
{
    return playback_.session();
}

PlaybackStatusSnapshot PlaybackStatusQueryService::snapshot() const
{
    const auto& session_value = playback_.session();
    PlaybackStatusSnapshot result{};
    result.status = session_value.status;
    result.current_track_id = session_value.current_track_id;
    result.elapsed_seconds = session_value.elapsed_seconds;
    result.shuffle_enabled = session_value.shuffle_enabled;
    result.repeat_all = session_value.repeat_all;
    result.repeat_one = session_value.repeat_one;

    if (session_value.current_track_id) {
        if (const auto* track = library_.findTrack(*session_value.current_track_id)) {
            result.title = track->title;
            result.source_label = track->remote && !track->source_label.empty() ? track->source_label : "LOCAL";
            result.duration_seconds = track->duration_seconds;
        }
    } else if (!session_value.current_stream_title.empty()) {
        result.title = session_value.current_stream_title;
        result.source_label = session_value.current_stream_source.empty() ? "STREAM" : session_value.current_stream_source;
        result.duration_seconds = session_value.current_stream_duration_seconds;
    }
    return result;
}

QueueStatusSnapshot PlaybackStatusQueryService::queueSnapshot() const
{
    const auto& session_value = playback_.session();
    const auto& queue = playback_.queue();
    QueueStatusSnapshot result{};
    result.active_ids = queue.active_ids;
    result.active_index = queue.active_index;
    result.shuffle_enabled = session_value.shuffle_enabled;
    result.repeat_all = session_value.repeat_all;
    result.repeat_one = session_value.repeat_one;
    return result;
}

} // namespace lofibox::application
