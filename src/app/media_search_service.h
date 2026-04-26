// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>
#include <vector>

#include "app/library_model.h"
#include "app/media_item.h"
#include "app/remote_media_services.h"

namespace lofibox::app {

struct MediaSearchResult {
    std::vector<MediaItem> local_items{};
    std::vector<MediaItem> remote_items{};
};

class MediaSearchService {
public:
    [[nodiscard]] static MediaSearchResult searchLocal(const LibraryModel& library, std::string_view query, int limit);
    [[nodiscard]] static std::vector<MediaItem> mapRemoteResults(const RemoteServerProfile& profile, const std::vector<RemoteTrack>& tracks);
};

} // namespace lofibox::app
