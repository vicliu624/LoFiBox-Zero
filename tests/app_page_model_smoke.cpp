// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "app/app_page_model.h"

int main()
{
    lofibox::app::AppPageModelInput input{};

    input.page = lofibox::app::AppPage::Settings;
    input.network_connected = true;
    input.metadata_display_name = "BUILT-IN";
    input.settings.backlight_index = 2;
    const auto settings_model = lofibox::app::buildAppPageModel(input);
    if (settings_model.title != "SETTINGS" || settings_model.rows.size() != 8 || settings_model.rows[0].second != "ONLINE" || settings_model.rows[3].second != "3") {
        std::cerr << "Expected Settings page model to own settings rows.\n";
        return 1;
    }

    input.page = lofibox::app::AppPage::Songs;
    input.library_title_override = "recently added";
    input.library_rows = std::vector<std::pair<std::string, std::string>>{{"Song A", "Artist A"}};
    const auto songs_model = lofibox::app::buildAppPageModel(input);
    if (songs_model.title != "RECENTLY ADDED" || !songs_model.browse_list || songs_model.rows.size() != 1) {
        std::cerr << "Expected library page model to preserve title override, rows, and browse-list state.\n";
        return 1;
    }

    input.page = lofibox::app::AppPage::NowPlaying;
    input.library_title_override.reset();
    input.library_rows.reset();
    const auto now_playing_model = lofibox::app::buildAppPageModel(input);
    if (now_playing_model.title != "NOW PLAYING" || now_playing_model.browse_list || now_playing_model.max_visible_rows != lofibox::app::kPageModelMaxVisibleRows) {
        std::cerr << "Expected non-list page defaults to be modeled consistently.\n";
        return 1;
    }

    input.page = lofibox::app::AppPage::SourceManager;
    input.source_manager_rows = std::vector<std::pair<std::string, std::string>>{{"ADD Navidrome", "opensubsonic"}};
    const auto source_model = lofibox::app::buildAppPageModel(input);
    if (source_model.title != "SOURCES" || !source_model.browse_list || source_model.rows.size() != 1) {
        std::cerr << "Expected Source Manager to be a projected browse list.\n";
        return 1;
    }

    return 0;
}
