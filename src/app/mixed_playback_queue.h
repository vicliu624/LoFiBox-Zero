// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <vector>

#include "app/media_item.h"

namespace lofibox::app {

class MixedPlaybackQueue {
public:
    void replace(std::vector<MediaItem> items, int selected_index = 0);
    [[nodiscard]] const std::vector<MediaItem>& items() const noexcept;
    [[nodiscard]] std::optional<MediaItem> current() const;
    [[nodiscard]] int currentIndex() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::optional<MediaItem> step(int delta);

private:
    std::vector<MediaItem> items_{};
    int current_index_{0};
};

} // namespace lofibox::app
