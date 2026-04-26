// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "app/library_model.h"

namespace lofibox::app {

class LibraryQueryService {
public:
    [[nodiscard]] static const TrackRecord* findTrack(const LibraryModel& library, int id) noexcept;
    [[nodiscard]] static TrackRecord* findMutableTrack(LibraryModel& library, int id) noexcept;
    [[nodiscard]] static std::vector<int> sortedTrackIds(const LibraryModel& library, SongSortMode sort_mode);
    [[nodiscard]] static std::vector<int> idsForGenre(const LibraryModel& library, const std::string& genre);
    [[nodiscard]] static std::vector<int> idsForComposer(const LibraryModel& library, const std::string& composer);
    [[nodiscard]] static std::vector<AlbumRecord> visibleAlbums(const LibraryModel& library, const AlbumsContext& context);
};

} // namespace lofibox::app
