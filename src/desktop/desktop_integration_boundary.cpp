// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktop/desktop_integration_boundary.h"

#include <utility>

namespace lofibox::desktop {

app::UserAction DesktopCommandAdapter::toUserAction(DesktopCommand command) const noexcept
{
    switch (command) {
    case DesktopCommand::PlayPause: return app::UserAction::Confirm;
    case DesktopCommand::Next: return app::UserAction::NextTrack;
    case DesktopCommand::Previous: return app::UserAction::Left;
    case DesktopCommand::Stop: return app::UserAction::Back;
    case DesktopCommand::OpenFiles:
    case DesktopCommand::OpenUrl:
        return app::UserAction::Confirm;
    }
    return app::UserAction::None;
}

DesktopNowPlayingProjection buildDesktopNowPlayingProjection(
    const app::PlaybackSession& session,
    const app::TrackRecord* track)
{
    DesktopNowPlayingProjection projection{};
    projection.title = track == nullptr ? "NO TRACK" : track->title;
    projection.artist = track == nullptr ? "" : track->artist;
    projection.playing = session.status == app::PlaybackStatus::Playing;
    return projection;
}

DesktopMprisProjection buildDesktopMprisProjection(const app::PlaybackSession& session)
{
    DesktopMprisProjection projection{};
    switch (session.status) {
    case app::PlaybackStatus::Playing:
        projection.playback_status = "Playing";
        break;
    case app::PlaybackStatus::Paused:
        projection.playback_status = "Paused";
        break;
    case app::PlaybackStatus::Empty:
        projection.playback_status = "Stopped";
        break;
    }
    return projection;
}

DesktopNotificationProjection buildDesktopNotificationProjection(
    const app::PlaybackSession& session,
    const app::TrackRecord* track)
{
    DesktopNotificationProjection projection{};
    if (session.status != app::PlaybackStatus::Playing || track == nullptr) {
        return projection;
    }
    projection.summary = track->title;
    projection.body = track->artist.empty() ? track->album : track->artist;
    projection.should_show = true;
    return projection;
}

DesktopRuntimeIntegrationState buildDesktopRuntimeIntegrationState(
    bool dbus_available,
    bool media_keys_available,
    bool notifications_available,
    std::vector<std::string> uris)
{
    DesktopRuntimeIntegrationState state{};
    state.dbus_available = dbus_available;
    state.media_keys_available = media_keys_available;
    state.notifications_available = notifications_available;
    state.pending_open_request.uris = std::move(uris);
    return state;
}

} // namespace lofibox::desktop
