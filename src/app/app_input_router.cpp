// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_input_router.h"

#include <cctype>

#include "app/input_actions.h"
#include "app/page_screen_keys.h"

namespace lofibox::app {
namespace {

[[nodiscard]] bool isLyricsToggle(const InputEvent& event) noexcept
{
    const auto ch = singleAsciiText(event);
    return ch && std::toupper(static_cast<unsigned char>(*ch)) == 'L';
}

[[nodiscard]] bool isQueueToggle(const InputEvent& event) noexcept
{
    const auto ch = singleAsciiText(event);
    return ch && std::toupper(static_cast<unsigned char>(*ch)) == 'Q';
}

[[nodiscard]] bool isSortShortcut(const InputEvent& event) noexcept
{
    const auto ch = singleAsciiText(event);
    return ch && std::toupper(static_cast<unsigned char>(*ch)) == 'T';
}

[[nodiscard]] bool isPlaylistEditShortcut(const InputEvent& event) noexcept
{
    const auto ch = singleAsciiText(event);
    return ch && std::toupper(static_cast<unsigned char>(*ch)) == 'E';
}

[[nodiscard]] bool isAudioEffectShortcut(const InputEvent& event) noexcept
{
    const auto ch = singleAsciiText(event);
    return ch && std::toupper(static_cast<unsigned char>(*ch)) == 'R';
}

[[nodiscard]] bool routeGlobalTransportShortcut(AppInputTarget& target, const InputEvent& event)
{
    switch (event.key) {
    case InputKey::F2:
        target.playFromMenu();
        return true;
    case InputKey::F3:
        target.pausePlayback();
        return true;
    case InputKey::F4:
        target.stepTrack(-1);
        return true;
    case InputKey::F5:
        target.stepTrack(1);
        return true;
    case InputKey::F6:
        target.cycleMainMenuPlaybackMode();
        return true;
    case InputKey::F7:
    case InputKey::F8:
        return false;
    case InputKey::F9:
        target.openSearchPage();
        return true;
    case InputKey::F10:
        target.openLibraryPage();
        return true;
    case InputKey::F11:
        target.openQueuePage();
        return true;
    case InputKey::F12:
        target.openSettingsPage();
        return true;
    default:
        return false;
    }
}

void executeScreenKey(AppInputTarget& target, ScreenKeyAction action)
{
    using A = ScreenKeyAction;
    switch (action) {
    case A::None: return;
    case A::Back: target.popPage(); return;
    case A::MenuPrevious: target.moveMainMenuSelection(-1); return;
    case A::MenuOpen: target.confirmMainMenu(); return;
    case A::MenuNext: target.moveMainMenuSelection(1); return;
    case A::NowPlaying:
        // Lyrics is entered from the player; return instead of growing the stack.
        if (target.currentPage() == AppPage::Lyrics) target.popPage();
        if (target.currentPage() != AppPage::NowPlaying) target.pushPage(AppPage::NowPlaying);
        return;
    case A::Search: target.openSearchPage(); return;
    case A::PageUp: target.moveSelectionPage(-1); return;
    case A::Open:
    case A::PlaySelection: target.confirmListPage(); return;
    case A::PageDown: target.moveSelectionPage(1); return;
    case A::Sort: target.cycleSongSortModeAndClamp(); return;
    case A::PlaybackMode: target.cycleMainMenuPlaybackMode(); return;
    case A::PreviousTrack: target.stepTrack(-1); return;
    case A::PlayPause:
        if (!target.nowPlayingConfirmBlocked()) target.togglePlayPause();
        return;
    case A::NextTrack: target.stepTrack(1); return;
    case A::Lyrics: target.pushPage(AppPage::Lyrics); return;
    case A::Queue:
        if (target.currentPage() != AppPage::Queue) target.openQueuePage();
        return;
    case A::PreviousBand: target.moveEqualizerSelection(-1); return;
    case A::GainDown: target.adjustSelectedEqualizerBand(-1); return;
    case A::Preset: target.cycleEqualizerPreset(1); return;
    case A::GainUp: target.adjustSelectedEqualizerBand(1); return;
    case A::NextBand: target.moveEqualizerSelection(1); return;
    case A::Library: target.openLibraryPage(); return;
    case A::Settings: target.openSettingsPage(); return;
    case A::Help: target.toggleHelpForCurrentPage(); return;
    }
}

[[nodiscard]] bool routeScreenKey(AppInputTarget& target, const InputEvent& event)
{
    const auto key = singleAsciiText(event);
    if (!key || *key < '4' || *key > '8') return false;
    const auto keys = screenKeysForPage(target.currentPage());
    // One physical press triggers at most one action, including page changes.
    if (!event.repeated) executeScreenKey(target, keys[static_cast<std::size_t>(*key - '4')].action);
    return true;
}

void routePointerTap(AppInputTarget& target, const InputEvent& event)
{
    constexpr int kTopbarHeight = 20;
    constexpr int kMainMenuLeftStart = 22;
    constexpr int kMainMenuLeftEnd = 90;
    constexpr int kMainMenuCenterStart = 112;
    constexpr int kMainMenuCenterEnd = 208;
    constexpr int kMainMenuRightStart = 230;
    constexpr int kMainMenuRightEnd = 298;
    constexpr int kMainMenuCardTop = 28;
    constexpr int kMainMenuCardBottom = 124;

    if (target.currentPage() == AppPage::Boot) return;
    if (target.helpOpen()) {
        if (event.y >= 0 && event.y < kTopbarHeight && event.x >= 0 && event.x < 96) target.closeHelp();
        return;
    }
    if (event.y >= kScreenKeyBarTop && event.y < 170 && event.x >= 21 && event.x < 301) {
        const auto index = static_cast<std::size_t>((event.x - 21) / 56);
        if (target.currentPage() == AppPage::Search || target.currentPage() == AppPage::RemoteFieldEditor) {
            if (index == 2) routeInput(target, InputEvent{InputKey::Enter, "OK"});
            else if (index == 4) routeInput(target, InputEvent{InputKey::Escape, "ESC"});
            return;
        }
        executeScreenKey(target, screenKeysForPage(target.currentPage())[index].action);
        return;
    }

    if (event.y >= 0 && event.y < kTopbarHeight && event.x >= 0 && event.x < 96) {
        target.toggleHelpForCurrentPage();
        return;
    }

    switch (target.currentPage()) {
    case AppPage::MainMenu:
        if (event.y < kMainMenuCardTop || event.y > kMainMenuCardBottom) {
            return;
        }
        if (event.x >= kMainMenuLeftStart && event.x <= kMainMenuLeftEnd) {
            target.moveMainMenuSelection(-1);
            target.confirmMainMenu();
        } else if (event.x >= kMainMenuCenterStart && event.x <= kMainMenuCenterEnd) {
            target.confirmMainMenu();
        } else if (event.x >= kMainMenuRightStart && event.x <= kMainMenuRightEnd) {
            target.moveMainMenuSelection(1);
            target.confirmMainMenu();
        }
        return;
    default:
        return;
    }
}

} // namespace

void routeInput(AppInputTarget& target, const InputEvent& event)
{
    if (event.isPointerTap()) {
        routePointerTap(target, event);
        return;
    }

    auto action = mapInput(event);
    const auto page = target.currentPage();
    const bool editing_text = page == AppPage::Search || page == AppPage::RemoteFieldEditor;

    if (page == AppPage::Boot) {
        return;
    }
    if (event.repeated && ((event.key >= InputKey::F1 && event.key <= InputKey::F12)
        || event.key == InputKey::MediaPlayPause || event.key == InputKey::MediaPrevious
        || event.key == InputKey::MediaNext)) return;

    // Firmware translates Cardputer Fn combinations to standard media keycodes.
    // Dedicated media controls remain usable while a help/editor is open.
    if (event.key == InputKey::MediaPlayPause) {
        target.togglePlayPause();
        return;
    }
    if (event.key == InputKey::MediaPrevious || event.key == InputKey::MediaNext) {
        target.stepTrack(event.key == InputKey::MediaPrevious ? -1 : 1);
        return;
    }

    if (event.key == InputKey::F1) {
        target.toggleHelpForCurrentPage();
        return;
    }

    if (target.helpOpen()) {
        if (action == UserAction::Back || event.key == InputKey::Enter) {
            target.closeHelp();
        }
        return;
    }

    if (page == AppPage::RemoteFieldEditor) {
        if (event.key == InputKey::Character && !event.text.empty()) {
            target.appendRemoteProfileEditText(event.text);
        } else if (event.key == InputKey::Backspace) {
            target.backspaceRemoteProfileEdit();
        } else if (event.key == InputKey::Enter) {
            target.commitRemoteProfileEdit();
        } else if (action == UserAction::Back || action == UserAction::Home) {
            // Escape cancels without committing; Backspace only edits text.
            if (action == UserAction::Home) target.showMainMenuPage();
            else target.popPage();
        }
        return;
    }

    if (!editing_text) {
        if (routeScreenKey(target, event)) return;
        if (const auto key = singleAsciiText(event)) {
            switch (std::toupper(static_cast<unsigned char>(*key))) {
            case 'F': action = UserAction::Up; break;
            case 'Z': action = UserAction::Left; break;
            case 'X': action = UserAction::Down; break;
            case 'C': action = UserAction::Right; break;
            default: break;
            }
        }
    }

    // Desktop keyboards normally use Space, and dedicated media buttons use
    // XF86AudioPlay/XF86AudioPause.  Keep literal spaces available while
    // editing a search query, but treat them as a transport toggle elsewhere.
    if (page != AppPage::Search && singleAsciiText(event) == std::optional<char>{' '}) {
        target.togglePlayPause();
        return;
    }

    if (routeGlobalTransportShortcut(target, event)) {
        return;
    }

    if (page != AppPage::Search && isAudioEffectShortcut(event)) {
        target.cycleAudioEffect();
        return;
    }

    if (page == AppPage::NowPlaying) {
        if (action == UserAction::Back || action == UserAction::Home) {
            if (action == UserAction::Home) {
                target.showMainMenuPage();
            } else {
                target.popPage();
            }
        } else if (isLyricsToggle(event)) {
            target.pushPage(AppPage::Lyrics);
        } else if (isQueueToggle(event)) {
            target.pushPage(AppPage::Queue);
        } else if (action == UserAction::Left) {
            target.stepTrack(-1);
        } else if (action == UserAction::Right || action == UserAction::NextTrack) {
            target.stepTrack(1);
        } else if (action == UserAction::Up || action == UserAction::Down) {
            target.cycleMainMenuPlaybackMode();
        } else if (action == UserAction::Confirm && !target.nowPlayingConfirmBlocked()) {
            target.togglePlayPause();
        }
        return;
    }

    if (page == AppPage::Lyrics) {
        if (action == UserAction::Back || action == UserAction::Home) {
            if (action == UserAction::Home) {
                target.showMainMenuPage();
            } else {
                target.popPage();
            }
        } else if (isLyricsToggle(event)) {
            target.popPage();
        } else if (action == UserAction::Left) {
            target.stepTrack(-1);
        } else if (action == UserAction::Right || action == UserAction::NextTrack) {
            target.stepTrack(1);
        }
        return;
    }

    if (page == AppPage::Equalizer) {
        if (action == UserAction::Back || action == UserAction::Home) {
            if (action == UserAction::Home) {
                target.showMainMenuPage();
            } else {
                target.popPage();
            }
        } else if (action == UserAction::Left) {
            target.moveEqualizerSelection(-1);
        } else if (action == UserAction::Right) {
            target.moveEqualizerSelection(1);
        } else if (action == UserAction::Up) {
            target.adjustSelectedEqualizerBand(1);
        } else if (action == UserAction::Down) {
            target.adjustSelectedEqualizerBand(-1);
        } else if (action == UserAction::PageUp) {
            target.adjustSelectedEqualizerBand(3);
        } else if (action == UserAction::PageDown) {
            target.adjustSelectedEqualizerBand(-3);
        } else if (action == UserAction::Confirm) {
            target.cycleEqualizerPreset(1);
        }
        return;
    }

    if (page == AppPage::Search) {
        if (event.key == InputKey::Character && !event.text.empty()) {
            target.appendSearchText(event.text);
            return;
        }
        if (event.key == InputKey::Backspace) {
            target.backspaceSearchQuery();
            return;
        }
    }

    if (target.isBrowseListPage()) {
        if ((event.key == InputKey::Insert || isPlaylistEditShortcut(event)) && (page == AppPage::Playlists || page == AppPage::PlaylistDetail)) {
            target.pushPage(AppPage::PlaylistEditor);
            return;
        }
        if (isSortShortcut(event) && (page == AppPage::Songs || page == AppPage::PlaylistDetail)) {
            target.cycleSongSortModeAndClamp();
            return;
        }
    }

    if (page == AppPage::MainMenu) {
        if (action == UserAction::Left) {
            target.moveMainMenuSelection(-1);
        } else if (action == UserAction::Right) {
            target.moveMainMenuSelection(1);
        } else if (action == UserAction::PageUp) {
            target.moveMainMenuSelection(-3);
        } else if (action == UserAction::PageDown) {
            target.moveMainMenuSelection(3);
        } else if (action == UserAction::Home) {
            target.resetMainMenuSelection();
        } else if (action == UserAction::Confirm) {
            target.confirmMainMenu();
        }
    } else if (action == UserAction::Back || action == UserAction::Home) {
        if (action == UserAction::Home) {
            target.showMainMenuPage();
            return;
        }
        target.popPage();
    } else if (action == UserAction::Up) {
        target.moveSelection(-1);
    } else if (action == UserAction::Down) {
        target.moveSelection(1);
    } else if (action == UserAction::PageUp) {
        target.moveSelectionPage(-1);
    } else if (action == UserAction::PageDown) {
        target.moveSelectionPage(1);
    } else if (action == UserAction::Confirm) {
        target.confirmListPage();
    }
}

} // namespace lofibox::app
