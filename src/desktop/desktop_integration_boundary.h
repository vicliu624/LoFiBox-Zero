// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "app/input_actions.h"
#include "app/library_model.h"
#include "app/playback_state.h"

namespace lofibox::desktop {

enum class DesktopCommand {
    PlayPause,
    Next,
    Previous,
};

struct DesktopNowPlayingProjection {
    std::string title{};
    std::string artist{};
    bool playing{false};
};

class DesktopCommandAdapter {
public:
    [[nodiscard]] app::UserAction toUserAction(DesktopCommand command) const noexcept;
};

[[nodiscard]] DesktopNowPlayingProjection buildDesktopNowPlayingProjection(
    const app::PlaybackSession& session,
    const app::TrackRecord* track);

} // namespace lofibox::desktop
