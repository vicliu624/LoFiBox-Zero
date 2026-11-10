// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_renderer.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include "app/app_projection_builder.h"
#include "app/page_screen_keys.h"
#include "core/bitmap_font.h"
#include "core/display_profile.h"
#include "ui/pages/about_page.h"
#include "ui/pages/equalizer_page.h"
#include "ui/pages/list_page.h"
#include "ui/pages/lyrics_page.h"
#include "ui/pages/main_menu_page.h"
#include "ui/pages/now_playing_page.h"
#include "ui/ui_primitives.h"

namespace lofibox::app {
namespace {

namespace ui_pages = lofibox::ui::pages;
using clock = std::chrono::steady_clock;

std::vector<std::pair<std::string_view, std::string_view>> helpRowsForPage(AppPage page)
{
    using Row = std::pair<std::string_view, std::string_view>;
    std::vector<Row> rows;
    if (page == AppPage::RemoteFieldEditor) {
        rows = {{"TYPE", "VALUE"}, {"BACKSPACE", "EDIT"}, {"OK", "SAVE"}, {"ESC", "CANCEL"}};
    } else if (page == AppPage::Search) {
        rows = {{"TYPE", "QUERY"}, {"BACKSPACE", "EDIT"}, {"FnF/FnX", "MOVE RESULTS"},
                {"FnL/FnM", "PAGE UP / DOWN"}, {"OK", "PLAY"}};
    } else {
        constexpr std::array<std::string_view, 5> numbers{"4", "5", "6", "7", "8"};
          const auto keys = screenKeysForPage(page);
            for (std::size_t index = 0; index < keys.size(); ++index) {
                if (keys[index].action != ScreenKeyAction::None) rows.emplace_back(numbers[index], keys[index].description);
            }
            rows.emplace_back(page == AppPage::Equalizer ? "F/X Z/C" : "FZXC / OK",
                              page == AppPage::Equalizer ? "GAIN / BAND" : "NAVIGATE / CONFIRM");
    }
    rows.emplace_back("FnQ/W/E", "MEDIA CONTROLS");
    rows.emplace_back("FnS/D/A", "VOL- / VOL+ / MUTE");
    if (page != AppPage::RemoteFieldEditor) rows.emplace_back("ESC", "BACK / CANCEL");
    rows.emplace_back("F1 / FnH", "HELP");
    return rows;
}

ui::ActionIcon screenKeyIcon(ScreenKeyAction action, bool playing)
{
    using A = ScreenKeyAction;
    using I = ui::ActionIcon;
    switch (action) {
    case A::Back: return I::Back;
    case A::MenuPrevious: case A::PreviousBand: return I::Left;
    case A::MenuNext: case A::NextBand: return I::Right;
    case A::MenuOpen: case A::Open: return I::Open;
    case A::NowPlaying: return I::Player;
    case A::Search: return I::Search;
    case A::PageUp: return I::PageUp;
    case A::PageDown: return I::PageDown;
    case A::PlaySelection: return I::Play;
    case A::PreviousTrack: return I::PreviousTrack;
    case A::PlayPause: return playing ? I::Pause : I::Play;
    case A::NextTrack: return I::NextTrack;
    case A::Lyrics: return I::Lyrics;
    case A::Queue: return I::Queue;
    case A::Sort: return I::Sort;
    case A::PlaybackMode: return I::Repeat;
    case A::GainDown: return I::Minus;
    case A::GainUp: return I::Plus;
    case A::Preset: return I::Equalizer;
    case A::Library: return I::Library;
    case A::Settings: return I::Settings;
    case A::None: case A::Help: return I::Help;
    }
    return I::Help;
}

void renderScreenKeys(core::Canvas& canvas, const AppRenderTarget& target)
{
    const auto page = target.currentPage();
    if (page == AppPage::Boot || target.helpOpen()) return;
    const auto& theme = target.theme();
    const bool playing = target.playbackSession().status == PlaybackStatus::Playing;
    const std::array colors{theme.palette.background, theme.palette.divider,
                           theme.palette.text_primary, theme.palette.text_secondary, theme.palette.progress};
    struct BarCache {
        core::Canvas image{core::kDisplayWidth, core::kDisplayHeight - kScreenKeyBarTop};
        AppPage page{AppPage::Boot};
        bool playing{};
        std::array<core::Color, 5> colors{};
        bool valid{};
    };
    // One bounded 35 KiB entry per render thread; page/theme/state changes
    // invalidate it. No growing text or image cache and no shared mutable state.
    thread_local BarCache cache;
    if (cache.valid && cache.page == page && cache.playing == playing && cache.colors == colors) {
        canvas.blit(cache.image, 0, kScreenKeyBarTop);
        return;
    }
    auto& bar = cache.image;
    bar.clear(theme.palette.background);
    bar.fillRect(21, 0, 280, 1, theme.palette.divider);
    if (page == AppPage::Search || page == AppPage::RemoteFieldEditor) {
        ui::drawActionIcon(bar, ui::ActionIcon::Keyboard, kScreenKeyCenters[0]-9, 5, theme.palette.text_secondary);
        ui::drawActionIcon(bar, page == AppPage::Search ? ui::ActionIcon::Play : ui::ActionIcon::Save,
                           kScreenKeyCenters[2]-9, 5, theme.palette.progress);
        ui::drawActionIcon(bar, ui::ActionIcon::Back, kScreenKeyCenters[4]-9, 5, theme.palette.text_secondary);
    } else {
        const auto keys = screenKeysForPage(page);
        for (std::size_t index = 0; index < keys.size(); ++index) {
            const auto& key = keys[index];
            if (key.action == ScreenKeyAction::None) continue;
            const bool primary = key.action == ScreenKeyAction::MenuOpen || key.action == ScreenKeyAction::Open
                || key.action == ScreenKeyAction::PlayPause || key.action == ScreenKeyAction::PlaySelection
                || key.action == ScreenKeyAction::Preset;
            ui::drawActionIcon(bar, screenKeyIcon(key.action, playing), kScreenKeyCenters[index]-9, 5,
                               primary ? theme.palette.progress : theme.palette.text_secondary);
        }
    }
    cache.page = page;
    cache.playing = playing;
    cache.colors = colors;
    cache.valid = true;
    canvas.blit(bar, 0, kScreenKeyBarTop);
}

void renderHelpIfOpen(core::Canvas& canvas, const AppRenderTarget& target)
{
    if (!target.helpOpen() || target.helpPage() == AppPage::Boot) {
        return;
    }
    const auto& theme = target.theme();
    const auto title = target.helpPage() == AppPage::MainMenu ? std::string_view{"MENU SHORTCUTS"} : std::string_view{"SHORTCUTS"};
    ui::drawPageHelpModal(canvas, theme, title, helpRowsForPage(target.helpPage()));
}

std::string trimBootText(std::string value, std::size_t max_size)
{
    if (value.size() <= max_size) {
        return value;
    }
    if (max_size <= 3U) {
        return value.substr(0, max_size);
    }
    return "..." + value.substr(value.size() - (max_size - 3U));
}

std::string bootStatusText(LibraryIndexState state, const LibraryScanProgress& progress)
{
    if (state == LibraryIndexState::Uninitialized) {
        return "STARTING";
    }
    if (state != LibraryIndexState::Loading) {
        return state == LibraryIndexState::Degraded ? "LIBRARY DEGRADED" : "LIBRARY READY";
    }

    switch (progress.phase) {
    case LibraryScanPhase::DiscoveringFiles: return "SCANNING FILES";
    case LibraryScanPhase::ReadingMetadata: return "READING METADATA";
    case LibraryScanPhase::BuildingIndexes: return "BUILDING INDEXES";
    case LibraryScanPhase::Complete: return "LIBRARY READY";
    case LibraryScanPhase::Failed: return "LIBRARY DEGRADED";
    case LibraryScanPhase::Idle: break;
    }
    return "LOADING LIBRARY";
}

std::string bootDetailText(const LibraryScanProgress& progress)
{
    switch (progress.phase) {
    case LibraryScanPhase::DiscoveringFiles:
        return "ROOTS " + std::to_string(progress.roots_scanned) + "/" + std::to_string(progress.roots_total)
            + "  FILES " + std::to_string(progress.files_discovered);
    case LibraryScanPhase::ReadingMetadata:
        return "FILES " + std::to_string(progress.files_processed) + "/" + std::to_string(progress.files_total)
            + "  TRACKS " + std::to_string(progress.tracks_indexed);
    case LibraryScanPhase::BuildingIndexes:
        return "TRACKS " + std::to_string(progress.tracks_indexed);
    case LibraryScanPhase::Failed:
        return progress.message.empty() ? "SCAN FAILED" : trimBootText(progress.message, 42U);
    case LibraryScanPhase::Complete:
        return "TRACKS " + std::to_string(progress.tracks_indexed);
    case LibraryScanPhase::Idle:
        break;
    }
    return {};
}

float bootProgressRatio(const LibraryScanProgress& progress)
{
    int total = 0;
    int done = 0;
    if (progress.phase == LibraryScanPhase::ReadingMetadata && progress.files_total > 0) {
        total = progress.files_total;
        done = progress.files_processed;
    } else if (progress.phase == LibraryScanPhase::DiscoveringFiles && progress.roots_total > 0) {
        total = progress.roots_total;
        done = progress.roots_scanned;
    } else if (progress.phase == LibraryScanPhase::BuildingIndexes || progress.phase == LibraryScanPhase::Complete) {
        total = 1;
        done = progress.phase == LibraryScanPhase::Complete ? 1 : 0;
    }
    if (total <= 0) {
        return 0.0F;
    }
    return std::clamp(static_cast<float>(done) / static_cast<float>(total), 0.0F, 1.0F);
}

void drawBootProgress(core::Canvas& canvas, const ui::UiTheme& theme, const LibraryScanProgress& progress)
{
    const float ratio = bootProgressRatio(progress);
    constexpr int width = 180;
    constexpr int height = 4;
    const int x = (core::kDisplayWidth - width) / 2;
    constexpr int y = 172;
    canvas.fillRect(x, y, width, height, theme.palette.panel2);
    canvas.fillRect(x, y, static_cast<int>(static_cast<float>(width) * ratio), height, theme.palette.progress);
}

void renderBootPage(core::Canvas& canvas, const AppRenderTarget& target)
{
    const auto& theme = target.theme();
    canvas.fillRect(0, 0, core::kDisplayWidth, core::kDisplayHeight, theme.palette.background);
    const auto progress = target.libraryScanProgress();
    const std::string status = bootStatusText(target.libraryState(), progress);
    if (target.assets().logo) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - target.bootStarted());
        const float t = std::clamp(static_cast<float>(elapsed.count()) / 1200.0f, 0.0f, 1.0f);
        const auto opacity = static_cast<std::uint8_t>(std::clamp(72.0f + (t * 183.0f), 72.0f, 255.0f));
        constexpr int size = 122;
        const int x = (core::kDisplayWidth - size) / 2;
        constexpr int y = 18;
        ui::blitScaledCanvas(canvas, *target.assets().logo, x, y, size, size, opacity);
    } else {
        ui::drawText(canvas, "LOFIBOX ZERO", ui::centeredX("LOFIBOX ZERO", 2), 38, theme.palette.text_primary, 2);
    }
    ui::drawText(canvas, status, ui::centeredX(status, 1), 144, theme.palette.text_secondary, 1);
    const auto detail = bootDetailText(progress);
    if (!detail.empty()) {
        ui::drawText(canvas, detail, ui::centeredX(detail, 1), 158, theme.palette.text_muted, 1);
        drawBootProgress(canvas, theme, progress);
    }
    if (!progress.current_path.empty() && progress.phase == LibraryScanPhase::ReadingMetadata) {
        const auto path = trimBootText(progress.current_path, 48U);
        ui::drawText(canvas, path, ui::centeredX(path, 1), 182, theme.palette.text_muted, 1);
    }
}

void renderMainMenu(core::Canvas& canvas, const AppRenderTarget& target)
{
    const auto& theme = target.theme();
    ui_pages::renderMainMenuPage(
        canvas,
        buildMainMenuProjection(target),
        theme);
}

void renderNowPlaying(core::Canvas& canvas, const AppRenderTarget& target)
{
    const auto& theme = target.theme();
    ui::drawListPageFrame(canvas, theme);
    const auto& playback = target.playbackSession();
    std::string source_label = playback.current_stream_source;
    if (playback.current_track_id) {
        if (const auto* track = target.findTrack(*playback.current_track_id); track && track->remote) {
            source_label = track->source_label;
        } else {
            source_label.clear();
        }
    }
    ui::drawTopBar(canvas, theme, target.pageModel().title, true, {}, source_label);
    ui_pages::renderNowPlayingPage(
        canvas,
        buildNowPlayingProjection(target),
        theme);
}

void renderLyrics(core::Canvas& canvas, const AppRenderTarget& target)
{
    const auto& theme = target.theme();
    ui::drawListPageFrame(canvas, theme);
    ui::drawTopBar(canvas, theme, target.pageModel().title, true);
    ui_pages::renderLyricsPage(
        canvas,
        buildLyricsProjection(target),
        theme);
}

void renderList(core::Canvas& canvas, const AppRenderTarget& target)
{
    const auto& theme = target.theme();
    ui_pages::renderListPage(
        canvas,
        buildListProjection(target),
        theme);
}

} // namespace

void renderApp(core::Canvas& canvas, const AppRenderTarget& target)
{
    const auto& theme = target.theme();
    canvas.clear(theme.palette.background);

    switch (target.currentPage()) {
    case AppPage::Boot:
        renderBootPage(canvas, target);
        return;
    case AppPage::MainMenu:
        renderMainMenu(canvas, target);
        break;
    case AppPage::NowPlaying:
        renderNowPlaying(canvas, target);
        break;
    case AppPage::Lyrics:
        renderLyrics(canvas, target);
        break;
    case AppPage::Equalizer:
        ui_pages::renderEqualizerPage(canvas, buildEqualizerProjection(target), theme);
        break;
    case AppPage::About:
        ui_pages::renderAboutPage(canvas, buildAboutProjection(target), theme);
        break;
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
    case AppPage::RemoteSetup:
    case AppPage::SourceManager:
    case AppPage::RemoteProfileSettings:
    case AppPage::RemoteFieldEditor:
    case AppPage::Search:
    case AppPage::Queue:
    case AppPage::RemoteBrowse:
    case AppPage::ServerDiagnostics:
    case AppPage::StreamDetail:
    case AppPage::PlaylistEditor:
        renderList(canvas, target);
        break;
    }
    renderScreenKeys(canvas, target);
    renderHelpIfOpen(canvas, target);
}

} // namespace lofibox::app
