// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include "app/library_model.h"

namespace lofibox::app {

struct LibraryListContext {
    AlbumsContext albums{};
    SongsContext songs{};
    PlaylistContext playlist{};
    std::vector<int> on_the_go_ids{};
    SongSortMode song_sort_mode{SongSortMode::TitleAscending};
};

} // namespace lofibox::app
