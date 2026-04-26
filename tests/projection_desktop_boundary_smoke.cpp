// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <vector>

#include "app/settings_projection_builder.h"
#include "app/source_manager_projection.h"
#include "desktop/desktop_integration_boundary.h"

int main()
{
    const auto settings_rows = lofibox::app::buildSettingsProjectionRows({
        true,
        "BUILT-IN",
        0,
        1,
        true});
    if (settings_rows.size() != 7 || settings_rows[0].second != "ONLINE" || settings_rows[5].second != "SERVERS") {
        std::cerr << "Expected Settings projection rows to expose network and remote source state.\n";
        return 1;
    }

    std::vector<lofibox::app::RemoteServerProfile> profiles{
        {lofibox::app::RemoteServerKind::Jellyfin, "jf", "Home Jellyfin", "https://jellyfin.example", "", "", ""},
        {lofibox::app::RemoteServerKind::Navidrome, "nav", "Home Navidrome", "https://navidrome.example", "", "", ""},
    };
    const auto source_rows = lofibox::app::buildSourceManagerRows(profiles);
    if (source_rows.size() != 5 || source_rows[4].second != "NAVIDROME") {
        std::cerr << "Expected Source Manager projection to include local/direct/radio and remote server rows.\n";
        return 1;
    }

    lofibox::desktop::DesktopCommandAdapter adapter{};
    if (adapter.toUserAction(lofibox::desktop::DesktopCommand::Next) != lofibox::app::UserAction::NextTrack) {
        std::cerr << "Expected desktop media next command to map to app action.\n";
        return 1;
    }
    if (adapter.toUserAction(lofibox::desktop::DesktopCommand::Stop) != lofibox::app::UserAction::Back) {
        std::cerr << "Expected desktop stop command to map to app back/stop-facing action.\n";
        return 1;
    }

    lofibox::app::PlaybackSession session{};
    session.status = lofibox::app::PlaybackStatus::Playing;
    lofibox::app::TrackRecord track{};
    track.title = "Desktop Song";
    track.artist = "Desktop Artist";
    const auto desktop_projection = lofibox::desktop::buildDesktopNowPlayingProjection(session, &track);
    if (!desktop_projection.playing || desktop_projection.title != "Desktop Song" || desktop_projection.artist != "Desktop Artist") {
        std::cerr << "Expected desktop now-playing projection to be derived from playback and library facts.\n";
        return 1;
    }
    const auto mpris_projection = lofibox::desktop::buildDesktopMprisProjection(session);
    if (mpris_projection.playback_status != "Playing" || !mpris_projection.can_play || !mpris_projection.can_pause) {
        std::cerr << "Expected desktop MPRIS projection to expose playback capabilities.\n";
        return 1;
    }
    const auto notification_projection = lofibox::desktop::buildDesktopNotificationProjection(session, &track);
    if (!notification_projection.should_show || notification_projection.summary != "Desktop Song") {
        std::cerr << "Expected desktop notification projection to be derived from now-playing facts.\n";
        return 1;
    }

    return 0;
}
