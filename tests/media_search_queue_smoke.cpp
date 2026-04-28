// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/media_search_service.h"
#include "playback/mixed_playback_queue.h"

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
    lofibox::app::TrackRecord cjk{};
    cjk.id = 8;
    cjk.title = "\xE6\x9D\xB1\xE4\xBA\xAC\xE3\x81\xAE\xE6\xAD\x8C";
    cjk.artist = "\xEB\xB0\xA4";
    cjk.album = "\xE6\xB5\xB7";
    library.tracks.push_back(cjk);

    const auto local_result = lofibox::app::MediaSearchService::searchLocal(library, "ocean", 5);
    assert(local_result.local_items.size() == 1U);
    assert(local_result.local_items.front().source.kind == lofibox::app::MediaItemSourceKind::LocalFile);

    const auto cjk_result = lofibox::app::MediaSearchService::searchLocal(library, "\xE6\x9D\xB1\xE4\xBA\xAC", 5);
    assert(cjk_result.local_items.size() == 1U);
    assert(cjk_result.local_items.front().local_track_id == 8);

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

