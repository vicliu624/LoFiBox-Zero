// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>

#include "app/library_model.h"
#include "playback/playback_state.h"

namespace lofibox::app {
class LibraryController;
class PlaybackController;
}

namespace lofibox::application {

struct PlaybackStatusSnapshot {
    app::PlaybackStatus status{app::PlaybackStatus::Paused};
    std::optional<int> current_track_id{};
    std::string title{};
    std::string source_label{};
    double elapsed_seconds{0.0};
    int duration_seconds{0};
    bool shuffle_enabled{false};
    bool repeat_all{false};
    bool repeat_one{false};
};

class PlaybackStatusQueryService {
public:
    PlaybackStatusQueryService(const app::PlaybackController& playback, const app::LibraryController& library) noexcept;

    [[nodiscard]] const app::PlaybackSession& session() const noexcept;
    [[nodiscard]] PlaybackStatusSnapshot snapshot() const;

private:
    const app::PlaybackController& playback_;
    const app::LibraryController& library_;
};

} // namespace lofibox::application
