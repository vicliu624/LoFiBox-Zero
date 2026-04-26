// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/lofibox_app.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/app_state.h"
#include "app/input_actions.h"
#include "app/library_controller.h"
#include "app/navigation_state.h"
#include "app/pages/about_page.h"
#include "app/pages/equalizer_page.h"
#include "app/pages/list_page.h"
#include "app/pages/lyrics_page.h"
#include "app/pages/main_menu_page.h"
#include "app/pages/now_playing_page.h"
#include "app/playback_controller.h"
#include "core/bitmap_font.h"
#include "core/color.h"
#include "core/display_profile.h"

namespace fs = std::filesystem;

namespace lofibox::app {
namespace {

using clock = std::chrono::steady_clock;
using core::rgba;

constexpr int kMainMenuItemCount = 6;

constexpr auto kBgRoot = rgba(5, 6, 8);
constexpr auto kBgPanel0 = rgba(10, 12, 15);
constexpr auto kBgPanel1 = rgba(16, 19, 24);
constexpr auto kBgPanel2 = rgba(23, 27, 33);
constexpr auto kChromeTopbar0 = rgba(43, 46, 51);
constexpr auto kChromeTopbar1 = rgba(16, 19, 23);
constexpr auto kDivider = rgba(42, 47, 54);
constexpr auto kTextPrimary = rgba(245, 247, 250);
constexpr auto kTextSecondary = rgba(199, 205, 211);
constexpr auto kTextMuted = rgba(141, 148, 156);
constexpr auto kFocusFill0 = rgba(95, 176, 255);
constexpr auto kFocusFill1 = rgba(47, 134, 255);
constexpr auto kFocusEdge = rgba(169, 219, 255);
constexpr auto kProgress = rgba(88, 168, 255);
constexpr auto kProgressStrong = rgba(47, 117, 255);
constexpr auto kEqHot0 = rgba(255, 178, 87);
constexpr auto kEqHot1 = rgba(255, 127, 42);
constexpr auto kEqSelected0 = rgba(127, 209, 255);
constexpr auto kEqSelected1 = rgba(47, 134, 255);
constexpr auto kGood = rgba(135, 217, 108);
constexpr auto kWarn = rgba(230, 179, 74);
constexpr auto kBad = rgba(214, 106, 95);

constexpr int kTopBarHeight = 20;
constexpr int kMaxVisibleRows = 6;
constexpr int kContentInset = 10;

constexpr std::string_view kUnknown = "UNKNOWN";
constexpr std::string_view kNoMusic = "NO MUSIC";
constexpr std::string_view kNoAlbums = "NO ALBUMS";
constexpr std::string_view kNoSongs = "NO SONGS";
constexpr std::string_view kNoGenres = "NO GENRES";
constexpr std::string_view kNoComposers = "NO COMPOSERS";
constexpr std::string_view kNoCompilations = "NO COMPILATIONS";
constexpr std::string_view kEmpty = "EMPTY";
constexpr std::string_view kNoTrack = "NO TRACK";
constexpr std::string_view kVersion = "0.1.0";

std::string upperText(std::string_view text)
{
    std::string result{};
    result.reserve(text.size());
    for (const unsigned char ch : text) {
        if (ch < 0x80U) {
            result.push_back(static_cast<char>(std::toupper(ch)));
        } else {
            result.push_back(static_cast<char>(ch));
        }
    }
    return result;
}

std::string fitText(std::string_view text, std::size_t max_chars);

std::string fitUpper(std::string_view text, std::size_t max_chars)
{
    return fitText(upperText(text), max_chars);
}

std::string fitText(std::string_view text, std::size_t max_chars)
{
    std::string result{text};
    std::size_t char_count = 0;
    for (const unsigned char ch : result) {
        if ((ch & 0xC0U) != 0x80U) {
            ++char_count;
        }
    }

    if (char_count <= max_chars) {
        return result;
    }

    if (max_chars <= 3) {
        std::size_t bytes = 0;
        std::size_t seen = 0;
        while (bytes < result.size() && seen < max_chars) {
            const unsigned char ch = static_cast<unsigned char>(result[bytes]);
            if ((ch & 0xC0U) != 0x80U) {
                ++seen;
            }
            ++bytes;
        }
        return result.substr(0, bytes);
    }

    std::size_t bytes = 0;
    std::size_t seen = 0;
    const std::size_t keep_chars = max_chars - 3;
    while (bytes < result.size() && seen < keep_chars) {
        const unsigned char ch = static_cast<unsigned char>(result[bytes]);
        if ((ch & 0xC0U) != 0x80U) {
            ++seen;
        }
        ++bytes;
    }

    while (bytes < result.size() && (static_cast<unsigned char>(result[bytes]) & 0xC0U) == 0x80U) {
        ++bytes;
    }

    return result.substr(0, bytes) + "...";
}

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
    case AppPage::About: return "ABOUT";
    }
    return "";
}

int centeredX(std::string_view text, int scale) noexcept
{
    return (core::kDisplayWidth - core::bitmap_font::measureText(text, scale)) / 2;
}

void drawText(core::Canvas& canvas, std::string_view text, int x, int y, core::Color color, int scale = 1)
{
    core::bitmap_font::drawText(canvas, text, x, y, color, scale);
}

void drawTopBar(core::Canvas& canvas, std::string_view title, bool show_back) noexcept
{
    canvas.fillRect(0, 0, core::kDisplayWidth, kTopBarHeight, kChromeTopbar0);
    canvas.fillRect(0, 12, core::kDisplayWidth, 8, kChromeTopbar1);
    canvas.fillRect(0, kTopBarHeight - 1, core::kDisplayWidth, 1, kDivider);

    if (show_back) {
        drawText(canvas, "<", 6, 6, kTextPrimary, 1);
    }

    drawText(canvas, title, centeredX(upperText(title), 1), 6, kTextPrimary, 1);
}

void drawListPageFrame(core::Canvas& canvas)
{
    canvas.fillRect(0, 0, core::kDisplayWidth, core::kDisplayHeight, kBgRoot);
    canvas.fillRect(0, kTopBarHeight, core::kDisplayWidth, core::kDisplayHeight - kTopBarHeight, kBgPanel0);
}

[[nodiscard]] core::Color alphaBlend(core::Color dst, core::Color src, std::uint8_t opacity) noexcept;

void drawPageHelpModal(core::Canvas& canvas, std::string_view title, const std::vector<std::pair<std::string_view, std::string_view>>& rows)
{
    constexpr int x = 26;
    constexpr int y = 28;
    constexpr int width = 268;
    constexpr int height = 112;

    canvas.fillRect(x + 4, y + 5, width, height, rgba(0, 0, 0));
    canvas.fillRect(x, y, width, height, rgba(12, 16, 22));
    canvas.strokeRect(x, y, width, height, rgba(116, 196, 255), 1);

    for (int col = 4; col < width - 4; ++col) {
        const float edge = static_cast<float>(std::min(col, width - 1 - col)) / 34.0f;
        const auto opacity = static_cast<std::uint8_t>(std::clamp(edge, 0.0f, 1.0f) * 96.0f);
        const auto top = alphaBlend(canvas.pixel(x + col, y + 3), rgba(148, 221, 255), opacity);
        const auto bottom = alphaBlend(canvas.pixel(x + col, y + height - 4), rgba(31, 91, 180), static_cast<std::uint8_t>(opacity * 0.45f));
        canvas.setPixel(x + col, y + 3, top);
        canvas.setPixel(x + col, y + height - 4, bottom);
    }

    drawText(canvas, title, centeredX(title, 1), y + 8, kTextPrimary, 1);
    if (rows.empty()) {
        drawText(canvas, "NO SHORTCUTS", centeredX("NO SHORTCUTS", 1), y + 54, kTextMuted, 1);
        return;
    }

    int row_y = y + 30;
    for (const auto& row : rows) {
        drawText(canvas, row.first, x + 20, row_y, kProgressStrong, 1);
        drawText(canvas, row.second, x + 70, row_y, kTextSecondary, 1);
        row_y += 14;
        if (row_y > y + height - 14) {
            break;
        }
    }
}

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

core::Color alphaBlend(core::Color dst, core::Color src, std::uint8_t opacity) noexcept
{
    const float src_alpha = (static_cast<float>(src.a) / 255.0f) * (static_cast<float>(opacity) / 255.0f);
    const float dst_alpha = static_cast<float>(dst.a) / 255.0f;
    const float out_alpha = src_alpha + (dst_alpha * (1.0f - src_alpha));

    if (out_alpha <= 0.0f) {
        return rgba(0, 0, 0, 0);
    }

    const auto blend = [&](std::uint8_t dst_c, std::uint8_t src_c) -> std::uint8_t {
        const float out =
            ((static_cast<float>(src_c) * src_alpha) + (static_cast<float>(dst_c) * dst_alpha * (1.0f - src_alpha))) / out_alpha;
        return static_cast<std::uint8_t>(std::clamp(out, 0.0f, 255.0f));
    };

    return rgba(
        blend(dst.r, src.r),
        blend(dst.g, src.g),
        blend(dst.b, src.b),
        static_cast<std::uint8_t>(std::clamp(out_alpha * 255.0f, 0.0f, 255.0f)));
}

void blitScaledCanvas(
    core::Canvas& target,
    const core::Canvas& source,
    int dst_x,
    int dst_y,
    int dst_w,
    int dst_h,
    std::uint8_t opacity = 255)
{
    if (dst_w <= 0 || dst_h <= 0 || source.width() <= 0 || source.height() <= 0) {
        return;
    }

    for (int y = 0; y < dst_h; ++y) {
        const int src_y = (y * source.height()) / dst_h;
        for (int x = 0; x < dst_w; ++x) {
            const int src_x = (x * source.width()) / dst_w;
            const auto src = source.pixel(src_x, src_y);
            if (src.a == 0 || opacity == 0) {
                continue;
            }

            const int tx = dst_x + x;
            const int ty = dst_y + y;
            const auto dst = target.pixel(tx, ty);
            target.setPixel(tx, ty, alphaBlend(dst, src, opacity));
        }
    }
}

} // namespace

struct LoFiBoxApp::Impl {
    std::vector<fs::path> media_roots{};
    AppAssets assets{};
    LibraryController library_controller{};
    bool boot_ready{false};
    clock::time_point boot_started{clock::now()};
    NavigationState navigation{};
    int main_menu_index{1};
    SettingsState settings{};
    NetworkState network{};
    MetadataServiceState metadata_service{};
    EqState eq{};
    PlaybackController playback_controller{};
    RuntimeServices services{};
    clock::time_point last_update{clock::now()};
    clock::time_point last_status_refresh{};
    clock::time_point now_playing_confirm_blocked_until{};
    bool help_open{false};
    AppPage help_page{AppPage::MainMenu};
    std::mt19937 random{12345U};

    [[nodiscard]] std::string networkStatusLabel() const
    {
        return network.connected ? "ONLINE" : "OFFLINE";
    }

    [[nodiscard]] bool bootAnimationComplete() const
    {
        if (!assets.logo) {
            return true;
        }
        return (clock::now() - boot_started) >= std::chrono::milliseconds{1450};
    }

    void refreshNetworkStatus()
    {
        network.connected = services.connectivity_provider->connected();
        network.status_message = network.connected ? "ONLINE" : "OFFLINE";
    }

    void refreshMetadataServiceState()
    {
        metadata_service.available = services.metadata_provider->available();
        metadata_service.display_name = services.metadata_provider->displayName();
        metadata_service.status = !metadata_service.available ? "UNAVAILABLE" : (network.connected ? "ONLINE" : "OFFLINE");
    }

    void refreshRuntimeStatusIfDue(bool force = false)
    {
        const auto now = clock::now();
        if (!force && last_status_refresh != clock::time_point{} && now - last_status_refresh < std::chrono::seconds{2}) {
            return;
        }
        last_status_refresh = now;
        refreshNetworkStatus();
        refreshMetadataServiceState();
    }

    [[nodiscard]] AppPage currentPage() const noexcept
    {
        return navigation.currentPage();
    }

    [[nodiscard]] const TrackRecord* findTrack(int id) const noexcept
    {
        return library_controller.findTrack(id);
    }

    [[nodiscard]] TrackRecord* findMutableTrack(int id) noexcept
    {
        return library_controller.findMutableTrack(id);
    }

    void resetListSelection() noexcept
    {
        navigation.resetListSelection();
    }

    void pushPage(AppPage page)
    {
        closeHelp();
        navigation.pushPage(page);
    }

    void popPage()
    {
        closeHelp();
        navigation.popPage();
    }

    void closeHelp() noexcept
    {
        help_open = false;
    }

    void toggleHelpForCurrentPage() noexcept
    {
        const auto page = currentPage();
        if (page == AppPage::Boot) {
            return;
        }
        if (help_open && help_page == page) {
            help_open = false;
            return;
        }
        help_page = page;
        help_open = true;
    }

    void playFromMenu()
    {
        const auto& session = playback_controller.session();
        if (session.status == PlaybackStatus::Paused && session.current_track_id) {
            playback_controller.resume();
            return;
        }
        if (session.status == PlaybackStatus::Playing) {
            return;
        }
        if (session.current_track_id) {
            playback_controller.resume();
            return;
        }

        const auto ids = library_controller.allSongIdsSorted();
        if (!ids.empty()) {
            library_controller.setSongsContextAll();
            (void)playback_controller.startTrack(library_controller, ids.front());
        }
    }

    void cycleMainMenuPlaybackMode()
    {
        const auto& session = playback_controller.session();
        if (session.shuffle_enabled) {
            playback_controller.setShuffleEnabled(false);
            playback_controller.setRepeatOne(true);
            return;
        }
        if (session.repeat_one) {
            playback_controller.setRepeatOne(false);
            playback_controller.setRepeatAll(false);
            return;
        }
        playback_controller.setRepeatOne(false);
        playback_controller.setRepeatAll(false);
        playback_controller.setShuffleEnabled(true);
    }

    [[nodiscard]] std::vector<std::pair<std::string_view, std::string_view>> helpRowsForPage(AppPage page) const
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
        case AppPage::Boot:
            return {};
        default:
            return {};
        }
    }

    void renderHelpIfOpen(core::Canvas& canvas) const
    {
        if (!help_open || help_page == AppPage::Boot) {
            return;
        }
        const auto title = help_page == AppPage::MainMenu ? std::string_view{"MENU SHORTCUTS"} : std::string_view{"SHORTCUTS"};
        drawPageHelpModal(canvas, title, helpRowsForPage(help_page));
    }

    [[nodiscard]] std::string pageTitle() const
    {
        const auto page = currentPage();
        if (const auto override = library_controller.titleOverrideForPage(page)) {
            return fitUpper(*override, 18);
        }
        return std::string(pageTitleDefault(page));
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>> currentRows() const
    {
        std::vector<std::pair<std::string, std::string>> rows{};
        if (const auto library_rows = library_controller.rowsForPage(currentPage())) {
            return *library_rows;
        }
        switch (currentPage()) {
        case AppPage::Settings:
            return {{"NETWORK", networkStatusLabel()}, {"METADATA", metadata_service.display_name}, {"SLEEP TIMER", settings.sleep_timer_index == 0 ? "OFF" : "ON"}, {"BACKLIGHT", std::to_string(settings.backlight_index + 1)}, {"LANGUAGE", "EN"}, {"REMOTE MEDIA", "SERVERS"}, {"ABOUT", "INFO"}};
        default:
            return rows;
        }
    }

    void clampListSelection()
    {
        navigation.clampListSelection(static_cast<int>(currentRows().size()), kMaxVisibleRows);
    }

    void moveSelection(int delta)
    {
        navigation.moveSelection(delta, static_cast<int>(currentRows().size()), kMaxVisibleRows);
    }

    [[nodiscard]] bool isBrowseListPage() const noexcept
    {
        switch (currentPage()) {
        case AppPage::MusicIndex:
        case AppPage::Artists:
        case AppPage::Albums:
        case AppPage::Songs:
        case AppPage::Genres:
        case AppPage::Composers:
        case AppPage::Compilations:
        case AppPage::Playlists:
        case AppPage::PlaylistDetail:
            return true;
        default:
            return false;
        }
    }

    bool nowPlayingConfirmBlocked() const
    {
        return clock::now() < now_playing_confirm_blocked_until;
    }

    void confirmMainMenu()
    {
        switch (main_menu_index) {
        case 0:
            library_controller.setSongsContextAll();
            pushPage(AppPage::Songs);
            break;
        case 1:
            pushPage(AppPage::MusicIndex);
            break;
        case 2:
            pushPage(AppPage::Playlists);
            break;
        case 3:
            pushPage(AppPage::NowPlaying);
            break;
        case 4:
            pushPage(AppPage::Equalizer);
            break;
        case 5:
            pushPage(AppPage::Settings);
            break;
        default:
            break;
        }
    }

    void confirmListPage()
    {
        const int selected = navigation.list_selection.selected;
        const auto page = currentPage();
        const auto library_result = library_controller.openSelectedListItem(page, selected);
        switch (library_result.kind) {
        case LibraryOpenResult::Kind::PushPage:
            pushPage(library_result.page);
            return;
        case LibraryOpenResult::Kind::StartTrack:
            if (playback_controller.startTrack(library_controller, library_result.track_id) && currentPage() != AppPage::NowPlaying) {
                pushPage(AppPage::NowPlaying);
            }
            return;
        case LibraryOpenResult::Kind::None:
            break;
        }

        switch (page) {
        case AppPage::Settings:
            if (selected == 6) {
                pushPage(AppPage::About);
            }
            break;
        default:
            break;
        }
    }
};

LoFiBoxApp::LoFiBoxApp(std::vector<std::filesystem::path> media_roots, AppAssets assets, RuntimeServices services)
    : impl_(std::make_unique<Impl>())
{
    impl_->media_roots = std::move(media_roots);
    impl_->assets = std::move(assets);
    impl_->services = withNullRuntimeServices(std::move(services));
    impl_->playback_controller.setServices(impl_->services);
    impl_->refreshRuntimeStatusIfDue(true);
}

LoFiBoxApp::~LoFiBoxApp() = default;

void LoFiBoxApp::update()
{
    const auto now = clock::now();
    const double delta = std::chrono::duration<double>(now - impl_->last_update).count();
    impl_->last_update = now;
    impl_->refreshRuntimeStatusIfDue();

    if (impl_->library_controller.state() == LibraryIndexState::Uninitialized) {
        impl_->library_controller.startLoading();
        return;
    }

    if (impl_->library_controller.state() == LibraryIndexState::Loading) {
        impl_->library_controller.refreshLibrary(impl_->media_roots, *impl_->services.metadata_provider);
    }

    if (impl_->currentPage() == AppPage::Boot
        && impl_->library_controller.state() != LibraryIndexState::Uninitialized
        && impl_->library_controller.state() != LibraryIndexState::Loading
        && impl_->bootAnimationComplete()) {
        impl_->navigation.replaceStack({AppPage::MainMenu});
    }

    impl_->playback_controller.update(delta, impl_->library_controller);
}

void LoFiBoxApp::handleInput(const InputEvent& event)
{
    const auto action = mapInput(event);
    const auto page = impl_->currentPage();

    if (page == AppPage::Boot) {
        return;
    }

    if (event.key == InputKey::F1) {
        impl_->toggleHelpForCurrentPage();
        return;
    }

    if (impl_->help_open) {
        if (action == UserAction::Back || event.key == InputKey::Enter) {
            impl_->closeHelp();
        }
        return;
    }

    if (page == AppPage::MainMenu) {
        if (event.key == InputKey::F2) {
            impl_->playFromMenu();
        } else if (event.key == InputKey::F3) {
            impl_->playback_controller.pause();
        } else if (event.key == InputKey::F4) {
            impl_->playback_controller.stepQueue(impl_->library_controller, -1);
        } else if (event.key == InputKey::F5) {
            impl_->playback_controller.stepQueue(impl_->library_controller, 1);
        } else if (event.key == InputKey::F6) {
            impl_->cycleMainMenuPlaybackMode();
        } else if (action == UserAction::Left) {
            impl_->main_menu_index = (impl_->main_menu_index + kMainMenuItemCount - 1) % kMainMenuItemCount;
        } else if (action == UserAction::Right) {
            impl_->main_menu_index = (impl_->main_menu_index + 1) % kMainMenuItemCount;
        } else if (action == UserAction::Home) {
            impl_->main_menu_index = 0;
        } else if (action == UserAction::Confirm) {
            impl_->confirmMainMenu();
        }
        return;
    }

    if (page == AppPage::NowPlaying) {
        if (action == UserAction::Back || action == UserAction::Home) {
            impl_->popPage();
        } else if (event.key == InputKey::Character && std::toupper(static_cast<unsigned char>(event.text)) == 'L') {
            impl_->pushPage(AppPage::Lyrics);
        } else if (action == UserAction::Left) {
            impl_->playback_controller.stepQueue(impl_->library_controller, -1);
        } else if (action == UserAction::Right || action == UserAction::NextTrack) {
            impl_->playback_controller.stepQueue(impl_->library_controller, 1);
        } else if (action == UserAction::Up) {
            impl_->playback_controller.toggleShuffle();
        } else if (action == UserAction::Down) {
            impl_->playback_controller.cycleRepeatMode();
        } else if (action == UserAction::Confirm && !impl_->nowPlayingConfirmBlocked()) {
            impl_->playback_controller.togglePlayPause();
        }
        return;
    }

    if (page == AppPage::Lyrics) {
        if (action == UserAction::Back || action == UserAction::Home) {
            impl_->popPage();
        } else if (event.key == InputKey::Character && std::toupper(static_cast<unsigned char>(event.text)) == 'L') {
            impl_->popPage();
        } else if (action == UserAction::Left) {
            impl_->playback_controller.stepQueue(impl_->library_controller, -1);
        } else if (action == UserAction::Right || action == UserAction::NextTrack) {
            impl_->playback_controller.stepQueue(impl_->library_controller, 1);
        }
        return;
    }

    if (page == AppPage::Equalizer) {
        if (action == UserAction::Back || action == UserAction::Home) {
            impl_->popPage();
        } else if (action == UserAction::Left) {
            impl_->eq.selected_band = std::max(0, impl_->eq.selected_band - 1);
        } else if (action == UserAction::Right) {
            impl_->eq.selected_band = std::min(static_cast<int>(impl_->eq.bands.size()) - 1, impl_->eq.selected_band + 1);
        } else if (action == UserAction::Up) {
            auto& band = impl_->eq.bands[static_cast<std::size_t>(impl_->eq.selected_band)];
            band = std::min(12, band + 1);
        } else if (action == UserAction::Down) {
            auto& band = impl_->eq.bands[static_cast<std::size_t>(impl_->eq.selected_band)];
            band = std::max(-12, band - 1);
        }
        return;
    }

    if (impl_->isBrowseListPage()) {
        if (event.key == InputKey::F3 && (page == AppPage::Songs || page == AppPage::PlaylistDetail)) {
            impl_->library_controller.cycleSongSortMode();
            impl_->clampListSelection();
            return;
        }
    }

    if (action == UserAction::Back || action == UserAction::Home) {
        impl_->popPage();
    } else if (action == UserAction::Up) {
        impl_->moveSelection(-1);
    } else if (action == UserAction::Down) {
        impl_->moveSelection(1);
    } else if (action == UserAction::Confirm) {
        impl_->confirmListPage();
    }
}

void LoFiBoxApp::render(core::Canvas& canvas) const
{
    canvas.clear(kBgRoot);

    const auto page = impl_->currentPage();
    if (page == AppPage::Boot) {
        canvas.fillRect(0, 0, core::kDisplayWidth, core::kDisplayHeight, kBgRoot);
        const std::string status = impl_->library_controller.state() == LibraryIndexState::Uninitialized
            ? "STARTING"
            : (impl_->library_controller.state() == LibraryIndexState::Loading ? "LOADING LIBRARY" : "LIBRARY READY");
        if (impl_->assets.logo) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - impl_->boot_started);
            const float t = std::clamp(static_cast<float>(elapsed.count()) / 1200.0f, 0.0f, 1.0f);
            const auto opacity = static_cast<std::uint8_t>(std::clamp(72.0f + (t * 183.0f), 72.0f, 255.0f));
            const int size = 122;
            const int x = (core::kDisplayWidth - size) / 2;
            const int y = 18;
            blitScaledCanvas(canvas, *impl_->assets.logo, x, y, size, size, opacity);
        } else {
            drawText(canvas, "LOFIBOX ZERO", centeredX("LOFIBOX ZERO", 2), 38, kTextPrimary, 2);
        }
        drawText(canvas, status, centeredX(status, 1), 144, kTextSecondary, 1);
        return;
    }

    if (page == AppPage::MainMenu) {
        const auto map_index_state = [](LibraryIndexState state) {
            switch (state) {
            case LibraryIndexState::Uninitialized: return pages::MenuIndexState::Uninitialized;
            case LibraryIndexState::Loading: return pages::MenuIndexState::Loading;
            case LibraryIndexState::Ready: return pages::MenuIndexState::Ready;
            case LibraryIndexState::Degraded: return pages::MenuIndexState::Degraded;
            }
            return pages::MenuIndexState::Uninitialized;
        };

        pages::renderMainMenuPage(
            canvas,
            pages::MainMenuView{
                impl_->main_menu_index,
                pages::MenuStorageView{impl_->library_controller.model().storage.available, impl_->library_controller.model().storage.capacity_bytes, impl_->library_controller.model().storage.free_bytes},
                map_index_state(impl_->library_controller.state()),
                impl_->network.connected,
                &impl_->assets,
                [&]() {
                    const auto& playback = impl_->playback_controller.session();
                    if (!playback.current_track_id) {
                        return std::string{"NO TRACK"};
                    }
                    const auto* track = impl_->findTrack(*playback.current_track_id);
                    if (track == nullptr) {
                        return std::string{"NO TRACK"};
                    }
                    std::string summary = playback.status == PlaybackStatus::Paused ? "PAUSED  " : "PLAYING  ";
                    summary += track->title.empty() ? "UNKNOWN" : track->title;
                    if (!track->artist.empty() && track->artist != "UNKNOWN") {
                        summary += " - ";
                        summary += track->artist;
                    }
                    return summary;
                }(),
                static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count() / 45)});
        impl_->renderHelpIfOpen(canvas);
        return;
    }

    if (page == AppPage::NowPlaying) {
        drawListPageFrame(canvas);
        drawTopBar(canvas, impl_->pageTitle(), true);
        const auto& playback = impl_->playback_controller.session();
        const TrackRecord* track = playback.current_track_id ? impl_->findTrack(*playback.current_track_id) : nullptr;
        const auto map_status = [](PlaybackStatus status) {
            switch (status) {
            case PlaybackStatus::Empty: return pages::NowPlayingStatus::Empty;
            case PlaybackStatus::Paused: return pages::NowPlayingStatus::Paused;
            case PlaybackStatus::Playing: return pages::NowPlayingStatus::Playing;
            }
            return pages::NowPlayingStatus::Empty;
        };
        pages::renderNowPlayingPage(canvas, pages::NowPlayingView{track != nullptr, track ? track->title : std::string{}, track ? track->artist : std::string{}, track ? track->album : std::string{}, track ? track->duration_seconds : 0, playback.elapsed_seconds, map_status(playback.status), playback.shuffle_enabled, playback.repeat_all, playback.repeat_one, playback.current_artwork ? &*playback.current_artwork : nullptr, playback.visualization_frame});
        impl_->renderHelpIfOpen(canvas);
        return;
    }

    if (page == AppPage::Lyrics) {
        drawListPageFrame(canvas);
        drawTopBar(canvas, impl_->pageTitle(), true);
        const auto& playback = impl_->playback_controller.session();
        const TrackRecord* track = playback.current_track_id ? impl_->findTrack(*playback.current_track_id) : nullptr;
        const auto map_status = [](PlaybackStatus status) {
            switch (status) {
            case PlaybackStatus::Empty: return pages::NowPlayingStatus::Empty;
            case PlaybackStatus::Paused: return pages::NowPlayingStatus::Paused;
            case PlaybackStatus::Playing: return pages::NowPlayingStatus::Playing;
            }
            return pages::NowPlayingStatus::Empty;
        };
        pages::renderLyricsPage(canvas, pages::LyricsPageView{track != nullptr, track ? track->title : std::string{}, track ? track->artist : std::string{}, track ? track->duration_seconds : 0, playback.elapsed_seconds, map_status(playback.status), playback.lyrics_lookup_pending, playback.current_lyrics, playback.visualization_frame});
        impl_->renderHelpIfOpen(canvas);
        return;
    }

    if (page == AppPage::Equalizer) {
        pages::renderEqualizerPage(canvas, pages::EqualizerPageView{impl_->eq.bands, impl_->eq.selected_band, impl_->eq.preset_name});
        impl_->renderHelpIfOpen(canvas);
        return;
    }

    if (page == AppPage::About) {
        pages::renderAboutPage(canvas, pages::AboutPageView{std::string(kVersion), formatStorage(impl_->library_controller.model().storage)});
        impl_->renderHelpIfOpen(canvas);
        return;
    }

    const auto rows = impl_->currentRows();
    std::string empty_label{};
    if (rows.empty()) {
        switch (page) {
        case AppPage::Artists: empty_label = std::string(kNoMusic); break;
        case AppPage::Albums: empty_label = std::string(kNoAlbums); break;
        case AppPage::Songs: empty_label = std::string(kNoSongs); break;
        case AppPage::Genres: empty_label = std::string(kNoGenres); break;
        case AppPage::Composers: empty_label = std::string(kNoComposers); break;
        case AppPage::Compilations: empty_label = std::string(kNoCompilations); break;
        case AppPage::PlaylistDetail: empty_label = std::string(kEmpty); break;
        default: empty_label = std::string(kEmpty); break;
        }
    }

    const bool browse_list = impl_->isBrowseListPage();
    pages::renderListPage(
        canvas,
        pages::ListPageView{
            impl_->pageTitle(),
            !browse_list,
            browse_list ? "F1:HELP" : "",
            rows,
            empty_label,
            impl_->navigation.list_selection.selected,
            impl_->navigation.list_selection.scroll,
            false});
    impl_->renderHelpIfOpen(canvas);

}

AppDebugSnapshot LoFiBoxApp::snapshot() const
{
    const auto& playback = impl_->playback_controller.session();
    return AppDebugSnapshot{
        impl_->currentPage(),
        impl_->library_controller.state() == LibraryIndexState::Ready || impl_->library_controller.state() == LibraryIndexState::Degraded,
        impl_->library_controller.model().tracks.size(),
        impl_->currentRows().size(),
        impl_->navigation.list_selection.selected,
        impl_->navigation.list_selection.scroll,
        playback.current_track_id.has_value(),
        playback.status,
        playback.current_track_id,
        playback.shuffle_enabled,
        playback.repeat_all,
        playback.repeat_one,
        impl_->network.connected,
        static_cast<int>(impl_->eq.bands.size()),
        impl_->eq.selected_band};
}

} // namespace lofibox::app
