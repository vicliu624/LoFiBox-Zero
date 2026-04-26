// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>

#include "app/library_model.h"
#include "app/remote_media_services.h"

namespace lofibox::app {

enum class MediaItemSourceKind {
    LocalFile,
    DirectUrl,
    Radio,
    RemoteServer,
};

struct MediaItemSource {
    MediaItemSourceKind kind{MediaItemSourceKind::LocalFile};
    std::string source_id{};
    std::string provider_family{};
};

struct MediaItem {
    std::string id{};
    std::optional<int> local_track_id{};
    std::string remote_track_id{};
    std::string title{};
    std::string artist{};
    std::string album{};
    int duration_seconds{0};
    MediaItemSource source{};
};

[[nodiscard]] MediaItem mediaItemFromTrackRecord(const TrackRecord& track);
[[nodiscard]] MediaItem mediaItemFromRemoteTrack(const RemoteServerProfile& profile, const RemoteTrack& track);

} // namespace lofibox::app
