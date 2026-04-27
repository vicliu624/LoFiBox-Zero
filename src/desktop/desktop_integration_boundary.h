// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "app/input_actions.h"
#include "app/library_model.h"
#include "playback/playback_state.h"

namespace lofibox::desktop {

enum class DesktopCommand {
    PlayPause,
    Next,
    Previous,
    Stop,
    OpenFiles,
    OpenUrl,
};

struct DesktopNowPlayingProjection {
    std::string title{};
    std::string artist{};
    bool playing{false};
};

struct DesktopMprisProjection {
    std::string playback_status{"Stopped"};
    std::string identity{"LoFiBox"};
    bool can_play{true};
    bool can_pause{true};
    bool can_go_next{true};
    bool can_go_previous{true};
};

struct DesktopNotificationProjection {
    std::string summary{};
    std::string body{};
    bool should_show{false};
};

struct DesktopOpenRequest {
    std::vector<std::string> uris{};
};

struct DesktopRuntimeIntegrationState {
    bool mpris_service_enabled{true};
    bool dbus_available{false};
    bool media_keys_available{false};
    bool notifications_available{false};
    DesktopOpenRequest pending_open_request{};
};

class DesktopCommandAdapter {
public:
    [[nodiscard]] app::UserAction toUserAction(DesktopCommand command) const noexcept;
};

[[nodiscard]] DesktopNowPlayingProjection buildDesktopNowPlayingProjection(
    const app::PlaybackSession& session,
    const app::TrackRecord* track);
[[nodiscard]] DesktopMprisProjection buildDesktopMprisProjection(const app::PlaybackSession& session);
[[nodiscard]] DesktopNotificationProjection buildDesktopNotificationProjection(
    const app::PlaybackSession& session,
    const app::TrackRecord* track);
[[nodiscard]] DesktopRuntimeIntegrationState buildDesktopRuntimeIntegrationState(
    bool dbus_available,
    bool media_keys_available,
    bool notifications_available,
    std::vector<std::string> uris = {});

} // namespace lofibox::desktop
