// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/mixed_playback_queue.h"

#include <algorithm>

namespace lofibox::app {

void MixedPlaybackQueue::replace(std::vector<MediaItem> items, int selected_index)
{
    items_ = std::move(items);
    if (items_.empty()) {
        current_index_ = 0;
        return;
    }
    current_index_ = std::clamp(selected_index, 0, static_cast<int>(items_.size()) - 1);
}

const std::vector<MediaItem>& MixedPlaybackQueue::items() const noexcept
{
    return items_;
}

std::optional<MediaItem> MixedPlaybackQueue::current() const
{
    if (items_.empty() || current_index_ < 0 || current_index_ >= static_cast<int>(items_.size())) {
        return std::nullopt;
    }
    return items_[static_cast<std::size_t>(current_index_)];
}

int MixedPlaybackQueue::currentIndex() const noexcept
{
    return current_index_;
}

bool MixedPlaybackQueue::empty() const noexcept
{
    return items_.empty();
}

std::optional<MediaItem> MixedPlaybackQueue::step(int delta)
{
    if (items_.empty()) {
        return std::nullopt;
    }
    current_index_ = (current_index_ + static_cast<int>(items_.size()) + delta) % static_cast<int>(items_.size());
    return current();
}

} // namespace lofibox::app

