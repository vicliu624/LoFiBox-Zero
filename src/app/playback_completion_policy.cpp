// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/playback_completion_policy.h"

#include <algorithm>

namespace lofibox::app {

std::optional<int> PlaybackCompletionPolicy::nextIndexAfterFinish(const QueueState& queue, const PlaybackSession& session) const noexcept
{
    if (queue.active_ids.empty() || queue.active_index < 0) {
        return std::nullopt;
    }
    const int last_index = static_cast<int>(queue.active_ids.size()) - 1;
    if (session.repeat_one) {
        return std::clamp(queue.active_index, 0, last_index);
    }
    if (queue.active_index < last_index) {
        return queue.active_index + 1;
    }
    if (session.repeat_all) {
        return 0;
    }
    return std::nullopt;
}

int PlaybackCompletionPolicy::steppedIndex(const QueueState& queue, const PlaybackSession& session, int delta) const noexcept
{
    if (queue.active_ids.empty()) {
        return -1;
    }
    int next = queue.active_index + delta;
    const int last_index = static_cast<int>(queue.active_ids.size()) - 1;
    if (next < 0) {
        next = session.repeat_all ? last_index : 0;
    }
    if (next > last_index) {
        next = session.repeat_all ? 0 : last_index;
    }
    return std::clamp(next, 0, last_index);
}

} // namespace lofibox::app
