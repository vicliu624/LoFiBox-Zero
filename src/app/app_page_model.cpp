// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_page_model.h"

#include <string_view>

#include "app/settings_projection_builder.h"
#include "ui/ui_primitives.h"

namespace lofibox::app {
namespace {

std::string_view pageTitleDefault(AppPage page) noexcept
{
    switch (page) {
    case AppPage::Boot: return "LOADING";
    case AppPage::MainMenu: return "LOFIBOX";
    case AppPage::MusicIndex: return "LIBRARY";
    case AppPage::Artists: return "ARTISTS";
    case AppPage::Albums: return "ALBUMS";
    case AppPage::Songs: return "SONGS";
    case AppPage::Genres: return "GENRES";
    case AppPage::Composers: return "COMPOSERS";
    case AppPage::Compilations: return "COMPILATIONS";
    case AppPage::Playlists: return "PLAYLISTS";
    case AppPage::PlaylistDetail: return "PLAYLIST";
    case AppPage::NowPlaying: return "NOW PLAYING";
    case AppPage::Lyrics: return "LYRICS";
    case AppPage::Equalizer: return "EQUALIZER";
    case AppPage::Settings: return "SETTINGS";
    case AppPage::SourceManager: return "SOURCES";
    case AppPage::About: return "ABOUT";
    }
    return "";
}

std::vector<std::pair<std::string, std::string>> settingsRows(const AppPageModelInput& input)
{
    return buildSettingsProjectionRows(SettingsProjectionInput{
        input.network_connected,
        input.metadata_display_name,
        input.settings.sleep_timer_index,
        input.settings.backlight_index,
        true});
}

} // namespace

bool isBrowseListPage(AppPage page) noexcept
{
    switch (page) {
    case AppPage::MusicIndex:
    case AppPage::Artists:
    case AppPage::Albums:
    case AppPage::Songs:
    case AppPage::Genres:
    case AppPage::Composers:
    case AppPage::Compilations:
    case AppPage::Playlists:
    case AppPage::PlaylistDetail:
    case AppPage::SourceManager:
        return true;
    default:
        return false;
    }
}

AppPageModel buildAppPageModel(const AppPageModelInput& input)
{
    AppPageModel model{};
    model.title = input.library_title_override ? ui::fitUpper(*input.library_title_override, 18) : std::string(pageTitleDefault(input.page));
    model.browse_list = isBrowseListPage(input.page);

    if (input.library_rows) {
        model.rows = *input.library_rows;
        return model;
    }

    if (input.page == AppPage::Settings) {
        model.rows = settingsRows(input);
    }
    if (input.page == AppPage::SourceManager && input.source_manager_rows) {
        model.rows = *input.source_manager_rows;
    }

    return model;
}

} // namespace lofibox::app
