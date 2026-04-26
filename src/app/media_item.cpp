// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/media_item.h"

#include "app/remote_profile_store.h"
#include "remote/common/remote_provider_contract.h"

namespace lofibox::app {

MediaItem mediaItemFromTrackRecord(const TrackRecord& track)
{
    MediaItem item{};
    item.id = "local:" + std::to_string(track.id);
    item.local_track_id = track.id;
    item.title = track.title;
    item.artist = track.artist;
    item.album = track.album;
    item.duration_seconds = track.duration_seconds;
    item.source.kind = MediaItemSourceKind::LocalFile;
    item.source.source_id = "local";
    item.source.provider_family = "local";
    return item;
}

MediaItem mediaItemFromRemoteTrack(const RemoteServerProfile& profile, const RemoteTrack& track)
{
    MediaItem item{};
    item.id = "remote:" + profile.id + ":" + track.id;
    item.remote_track_id = track.id;
    item.title = track.title;
    item.artist = track.artist;
    item.album = track.album;
    item.duration_seconds = track.duration_seconds;
    item.source.kind = MediaItemSourceKind::RemoteServer;
    item.source.source_id = profile.id;
    item.source.provider_family = std::string(remote::remoteProviderFamily(profile.kind));
    return item;
}

} // namespace lofibox::app
