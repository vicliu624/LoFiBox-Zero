// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_renderer.h"

#include <algorithm>
#include <cstdint>

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

constexpr std::string_view kNoMusic = "NO MUSIC";
constexpr std::string_view kNoAlbums = "NO ALBUMS";
constexpr std::string_view kNoSongs = "NO SONGS";
constexpr std::string_view kNoGenres = "NO GENRES";
constexpr std::string_view kNoComposers = "NO COMPOSERS";
constexpr std::string_view kNoCompilations = "NO COMPILATIONS";
constexpr std::string_view kEmpty = "EMPTY";
constexpr std::string_view kVersion = "0.1.0";

std::string formatStorage(const StorageInfo& storage)
{
    if (!storage.available || storage.capacity_bytes == 0) {
        return "N/A";
    }

    const auto used = storage.capacity_bytes - storage.free_bytes;
    const auto used_mb = static_cast<int>(used / (1024 * 1024));
    const auto total_mb = static_cast<int>(storage.capacity_bytes / (1024 * 1024));
    return std::to_string(used_mb) + "/" + std::to_string(total_mb) + "MB";
}

ui::SpectrumFrame toUiSpectrumFrame(const AudioVisualizationFrame& frame)
{
    ui::SpectrumFrame view{};
    view.available = frame.available;
    view.bands = frame.bands;
    return view;
}

ui::LyricsContent toUiLyricsContent(const TrackLyrics& lyrics)
{
    ui::LyricsContent view{};
    view.plain = lyrics.plain;
    view.synced = lyrics.synced;
    return view;
}

ui_pages::NowPlayingStatus toUiPlaybackStatus(PlaybackStatus status)
{
    switch (status) {
    case PlaybackStatus::Empty: return ui_pages::NowPlayingStatus::Empty;
    case PlaybackStatus::Paused: return ui_pages::NowPlayingStatus::Paused;
    case PlaybackStatus::Playing: return ui_pages::NowPlayingStatus::Playing;
    }
    return ui_pages::NowPlayingStatus::Empty;
}

ui_pages::MenuIndexState toUiIndexState(LibraryIndexState state)
{
    switch (state) {
    case LibraryIndexState::Uninitialized: return ui_pages::MenuIndexState::Uninitialized;
    case LibraryIndexState::Loading: return ui_pages::MenuIndexState::Loading;
    case LibraryIndexState::Ready: return ui_pages::MenuIndexState::Ready;
    case LibraryIndexState::Degraded: return ui_pages::MenuIndexState::Degraded;
    }
    return ui_pages::MenuIndexState::Uninitialized;
}

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

std::string playbackSummary(const AppRenderTarget& target)
{
    const auto& playback = target.playbackSession();
    if (!playback.current_track_id) {
        return "NO TRACK";
    }
    const auto* track = target.findTrack(*playback.current_track_id);
    if (track == nullptr) {
        return "NO TRACK";
    }
    std::string summary = playback.status == PlaybackStatus::Paused ? "PAUSED  " : "PLAYING  ";
    summary += track->title.empty() ? "UNKNOWN" : track->title;
    if (!track->artist.empty() && track->artist != "UNKNOWN") {
        summary += " - ";
        summary += track->artist;
    }
    return summary;
}

std::string emptyLabelForPage(AppPage page)
{
    switch (page) {
    case AppPage::Artists: return std::string(kNoMusic);
    case AppPage::Albums: return std::string(kNoAlbums);
    case AppPage::Songs: return std::string(kNoSongs);
    case AppPage::Genres: return std::string(kNoGenres);
    case AppPage::Composers: return std::string(kNoComposers);
    case AppPage::Compilations: return std::string(kNoCompilations);
    case AppPage::PlaylistDetail: return std::string(kEmpty);
    default: return std::string(kEmpty);
    }
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
        ui_pages::MainMenuView{
            target.mainMenuIndex(),
            ui_pages::MenuStorageView{target.storage().available, target.storage().capacity_bytes, target.storage().free_bytes},
            toUiIndexState(target.libraryState()),
            target.networkConnected(),
            &target.assets(),
            playbackSummary(target),
            static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count() / 45)});
    renderHelpIfOpen(canvas, target);
}

void renderNowPlaying(core::Canvas& canvas, const AppRenderTarget& target)
{
    ui::drawListPageFrame(canvas);
    ui::drawTopBar(canvas, target.pageModel().title, true);
    const auto& playback = target.playbackSession();
    const TrackRecord* track = playback.current_track_id ? target.findTrack(*playback.current_track_id) : nullptr;
    ui_pages::renderNowPlayingPage(
        canvas,
        ui_pages::NowPlayingView{
            track != nullptr,
            track ? track->title : std::string{},
            track ? track->artist : std::string{},
            track ? track->album : std::string{},
            track ? track->duration_seconds : 0,
            playback.elapsed_seconds,
            toUiPlaybackStatus(playback.status),
            playback.shuffle_enabled,
            playback.repeat_all,
            playback.repeat_one,
            playback.current_artwork ? &*playback.current_artwork : nullptr,
            toUiSpectrumFrame(playback.visualization_frame)});
    renderHelpIfOpen(canvas, target);
}

void renderLyrics(core::Canvas& canvas, const AppRenderTarget& target)
{
    ui::drawListPageFrame(canvas);
    ui::drawTopBar(canvas, target.pageModel().title, true);
    const auto& playback = target.playbackSession();
    const TrackRecord* track = playback.current_track_id ? target.findTrack(*playback.current_track_id) : nullptr;
    ui_pages::renderLyricsPage(
        canvas,
        ui_pages::LyricsPageView{
            track != nullptr,
            track ? track->title : std::string{},
            track ? track->artist : std::string{},
            track ? track->duration_seconds : 0,
            playback.elapsed_seconds,
            toUiPlaybackStatus(playback.status),
            playback.lyrics_lookup_pending,
            toUiLyricsContent(playback.current_lyrics),
            toUiSpectrumFrame(playback.visualization_frame)});
    renderHelpIfOpen(canvas, target);
}

void renderList(core::Canvas& canvas, const AppRenderTarget& target)
{
    const auto model = target.pageModel();
    const auto& rows = model.rows;
    const auto page = target.currentPage();
    ui_pages::renderListPage(
        canvas,
        ui_pages::ListPageView{
            model.title,
            !model.browse_list,
            model.browse_list ? "F1:HELP" : "",
            rows,
            rows.empty() ? emptyLabelForPage(page) : std::string{},
            target.listSelection().selected,
            target.listSelection().scroll,
            false});
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
        ui_pages::renderEqualizerPage(canvas, ui_pages::EqualizerPageView{target.eqState().bands, target.eqState().selected_band, target.eqState().preset_name});
        renderHelpIfOpen(canvas, target);
        return;
    case AppPage::About:
        ui_pages::renderAboutPage(canvas, ui_pages::AboutPageView{std::string(kVersion), formatStorage(target.storage())});
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
        renderList(canvas, target);
        return;
    }
}

} // namespace lofibox::app
