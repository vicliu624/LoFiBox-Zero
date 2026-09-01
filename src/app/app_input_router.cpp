// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_input_router.h"

#include <cctype>

#include "app/input_actions.h"

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
    case AppPage::NowPlaying:
        if (event.y < 112 || event.y > 146) {
            return;
        }
        if (event.x >= 104 && event.x < 144) {
            target.stepTrack(-1);
        } else if (event.x >= 144 && event.x < 190 && !target.nowPlayingConfirmBlocked()) {
            target.togglePlayPause();
        } else if (event.x >= 190 && event.x < 234) {
            target.stepTrack(1);
        } else if (event.x >= 234 && event.x < 276) {
            target.toggleShuffle();
        } else if (event.x >= 276 && event.x < 320) {
            target.cycleRepeatMode();
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

    const auto action = mapInput(event);
    const auto page = target.currentPage();

    if (page == AppPage::Boot) {
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
        }
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
