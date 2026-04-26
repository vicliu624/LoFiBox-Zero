// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktop/desktop_integration_boundary.h"

namespace lofibox::desktop {

app::UserAction DesktopCommandAdapter::toUserAction(DesktopCommand command) const noexcept
{
    switch (command) {
    case DesktopCommand::PlayPause: return app::UserAction::Confirm;
    case DesktopCommand::Next: return app::UserAction::NextTrack;
    case DesktopCommand::Previous: return app::UserAction::Left;
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

} // namespace lofibox::desktop
