// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_renderer.h"

#include <algorithm>
#include <cstdint>

#include "app/app_projection_builder.h"
#include "core/display_profile.h"
#include "ui/pages/about_page.h"
#include "ui/pages/equalizer_page.h"
#include "ui/pages/list_page.h"
#include "ui/pages/lyrics_page.h"
#include "ui/pages/main_menu_page.h"
#include "ui/pages/now_playing_page.h"
#include "ui/ui_primitives.h"
#include "ui/ui_theme.h"

namespace lofibox::app {
namespace {

namespace ui_pages = lofibox::ui::pages;
using clock = std::chrono::steady_clock;

std::vector<std::pair<std::string_view, std::string_view>> helpRowsForPage(AppPage page)
{
    switch (page) {
    case AppPage::MainMenu:
        return {
            {"F2", "PLAY SONG"},
            {"F3", "PAUSE"},
            {"F4", "PREVIOUS"},
            {"F5", "NEXT"},
            {"F6", "MODE: SHUFFLE/SEQ/ONE"},
        };
    case AppPage::Songs:
    case AppPage::PlaylistDetail:
        return {
            {"DEL", "DELETE SONG"},
            {"ENTER", "PLAY"},
            {"BACKSPACE", "BACK"},
            {"F2", "SEARCH"},
            {"F3", "SORT"},
        };
    case AppPage::Boot:
    case AppPage::MusicIndex:
    case AppPage::Artists:
    case AppPage::Albums:
    case AppPage::Genres:
    case AppPage::Composers:
    case AppPage::Compilations:
    case AppPage::Playlists:
    case AppPage::NowPlaying:
    case AppPage::Lyrics:
    case AppPage::Equalizer:
    case AppPage::Settings:
    case AppPage::SourceManager:
    case AppPage::About:
        return {};
    }
    return {};
}

void renderHelpIfOpen(core::Canvas& canvas, const AppRenderTarget& target)
{
    if (!target.helpOpen() || target.helpPage() == AppPage::Boot) {
        return;
    }
    const auto title = target.helpPage() == AppPage::MainMenu ? std::string_view{"MENU SHORTCUTS"} : std::string_view{"SHORTCUTS"};
    ui::drawPageHelpModal(canvas, title, helpRowsForPage(target.helpPage()));
}

void renderBootPage(core::Canvas& canvas, const AppRenderTarget& target)
{
    canvas.fillRect(0, 0, core::kDisplayWidth, core::kDisplayHeight, ui::kBgRoot);
    const std::string status = target.libraryState() == LibraryIndexState::Uninitialized
        ? "STARTING"
        : (target.libraryState() == LibraryIndexState::Loading ? "LOADING LIBRARY" : "LIBRARY READY");
    if (target.assets().logo) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - target.bootStarted());
        const float t = std::clamp(static_cast<float>(elapsed.count()) / 1200.0f, 0.0f, 1.0f);
        const auto opacity = static_cast<std::uint8_t>(std::clamp(72.0f + (t * 183.0f), 72.0f, 255.0f));
        constexpr int size = 122;
        const int x = (core::kDisplayWidth - size) / 2;
        constexpr int y = 18;
        ui::blitScaledCanvas(canvas, *target.assets().logo, x, y, size, size, opacity);
    } else {
        ui::drawText(canvas, "LOFIBOX ZERO", ui::centeredX("LOFIBOX ZERO", 2), 38, ui::kTextPrimary, 2);
    }
    ui::drawText(canvas, status, ui::centeredX(status, 1), 144, ui::kTextSecondary, 1);
}

void renderMainMenu(core::Canvas& canvas, const AppRenderTarget& target)
{
    ui_pages::renderMainMenuPage(
        canvas,
        buildMainMenuProjection(target));
    renderHelpIfOpen(canvas, target);
}

void renderNowPlaying(core::Canvas& canvas, const AppRenderTarget& target)
{
    ui::drawListPageFrame(canvas);
    ui::drawTopBar(canvas, target.pageModel().title, true);
    ui_pages::renderNowPlayingPage(
        canvas,
        buildNowPlayingProjection(target));
    renderHelpIfOpen(canvas, target);
}

void renderLyrics(core::Canvas& canvas, const AppRenderTarget& target)
{
    ui::drawListPageFrame(canvas);
    ui::drawTopBar(canvas, target.pageModel().title, true);
    ui_pages::renderLyricsPage(
        canvas,
        buildLyricsProjection(target));
    renderHelpIfOpen(canvas, target);
}

void renderList(core::Canvas& canvas, const AppRenderTarget& target)
{
    ui_pages::renderListPage(
        canvas,
        buildListProjection(target));
    renderHelpIfOpen(canvas, target);
}

} // namespace

void renderApp(core::Canvas& canvas, const AppRenderTarget& target)
{
    canvas.clear(ui::kBgRoot);

    switch (target.currentPage()) {
    case AppPage::Boot:
        renderBootPage(canvas, target);
        return;
    case AppPage::MainMenu:
        renderMainMenu(canvas, target);
        return;
    case AppPage::NowPlaying:
        renderNowPlaying(canvas, target);
        return;
    case AppPage::Lyrics:
        renderLyrics(canvas, target);
        return;
    case AppPage::Equalizer:
        ui_pages::renderEqualizerPage(canvas, buildEqualizerProjection(target));
        renderHelpIfOpen(canvas, target);
        return;
    case AppPage::About:
        ui_pages::renderAboutPage(canvas, buildAboutProjection(target));
        renderHelpIfOpen(canvas, target);
        return;
    case AppPage::MusicIndex:
    case AppPage::Artists:
    case AppPage::Albums:
    case AppPage::Songs:
    case AppPage::Genres:
    case AppPage::Composers:
    case AppPage::Compilations:
    case AppPage::Playlists:
    case AppPage::PlaylistDetail:
    case AppPage::Settings:
    case AppPage::SourceManager:
        renderList(canvas, target);
        return;
    }
}

} // namespace lofibox::app
