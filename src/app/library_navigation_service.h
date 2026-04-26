// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "app/app_page.h"
#include "app/library_list_context.h"
#include "app/library_model.h"

namespace lofibox::app {

class LibraryNavigationService {
public:
    [[nodiscard]] static std::optional<std::string> titleOverrideForPage(
        AppPage page,
        const LibraryListContext& context);

    [[nodiscard]] static std::optional<std::vector<std::pair<std::string, std::string>>> rowsForPage(
        AppPage page,
        const LibraryModel& library,
        const LibraryListContext& context,
        const std::vector<AlbumRecord>& visible_albums,
        const std::vector<int>& playlist_track_ids);
};

} // namespace lofibox::app
