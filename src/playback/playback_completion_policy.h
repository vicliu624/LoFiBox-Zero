// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include "playback/playback_state.h"

namespace lofibox::app {

class PlaybackCompletionPolicy {
public:
    [[nodiscard]] std::optional<int> nextIndexAfterFinish(const QueueState& queue, const PlaybackSession& session) const noexcept;
    [[nodiscard]] int steppedIndex(const QueueState& queue, const PlaybackSession& session, int delta) const noexcept;
};

} // namespace lofibox::app

