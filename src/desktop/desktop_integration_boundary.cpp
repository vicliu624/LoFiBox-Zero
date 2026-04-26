// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktop/desktop_integration_boundary.h"

namespace lofibox::desktop {

app::UserAction DesktopCommandAdapter::toUserAction(DesktopCommand command) const noexcept
{
    switch (command) {
    case DesktopCommand::PlayPause: return app::UserAction::Confirm;
    case DesktopCommand::Next: return app::UserAction::NextTrack;
    case DesktopCommand::Previous: return app::UserAction::Left;
    case DesktopCommand::Stop: return app::UserAction::Back;
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

} // namespace lofibox::desktop
