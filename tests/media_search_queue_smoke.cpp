// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/media_search_service.h"
#include "app/mixed_playback_queue.h"

#include <cassert>

int main()
{
    lofibox::app::LibraryModel library{};
    lofibox::app::TrackRecord local{};
    local.id = 7;
    local.title = "Ocean Song";
    local.artist = "Blue Artist";
    local.album = "Tides";
    library.tracks.push_back(local);

    const auto local_result = lofibox::app::MediaSearchService::searchLocal(library, "ocean", 5);
    assert(local_result.local_items.size() == 1U);
    assert(local_result.local_items.front().source.kind == lofibox::app::MediaItemSourceKind::LocalFile);

    lofibox::app::RemoteServerProfile profile{};
    profile.kind = lofibox::app::RemoteServerKind::Navidrome;
    profile.id = "nav";
    lofibox::app::RemoteTrack remote{};
    remote.id = "r1";
    remote.title = "Remote Song";
    remote.artist = "Remote Artist";
    const auto remote_items = lofibox::app::MediaSearchService::mapRemoteResults(profile, {remote});
    assert(remote_items.size() == 1U);
    assert(remote_items.front().source.kind == lofibox::app::MediaItemSourceKind::RemoteServer);
    assert(remote_items.front().source.provider_family == "opensubsonic");

    lofibox::app::MixedPlaybackQueue queue{};
    queue.replace({local_result.local_items.front(), remote_items.front()}, 0);
    assert(queue.current()->local_track_id == 7);
    assert(queue.step(1)->remote_track_id == "r1");
    assert(queue.step(1)->local_track_id == 7);
    assert(queue.step(-1)->remote_track_id == "r1");
    return 0;
}
