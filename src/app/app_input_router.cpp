// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_input_router.h"

#include <cctype>

#include "app/input_actions.h"

namespace lofibox::app {
namespace {

[[nodiscard]] bool isLyricsToggle(const InputEvent& event) noexcept
{
    return event.key == InputKey::Character && std::toupper(static_cast<unsigned char>(event.text)) == 'L';
}

[[nodiscard]] bool isQueueToggle(const InputEvent& event) noexcept
{
    return event.key == InputKey::Character && std::toupper(static_cast<unsigned char>(event.text)) == 'Q';
}

} // namespace

void routeInput(AppInputTarget& target, const InputEvent& event)
{
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

    if (page == AppPage::MainMenu) {
        if (event.key == InputKey::F2) {
            target.playFromMenu();
        } else if (event.key == InputKey::F3) {
            target.pausePlayback();
        } else if (event.key == InputKey::F4) {
            target.stepTrack(-1);
        } else if (event.key == InputKey::F5) {
            target.stepTrack(1);
        } else if (event.key == InputKey::F6) {
            target.cycleMainMenuPlaybackMode();
        } else if (action == UserAction::Left) {
            target.moveMainMenuSelection(-1);
        } else if (action == UserAction::Right) {
            target.moveMainMenuSelection(1);
        } else if (action == UserAction::Home) {
            target.resetMainMenuSelection();
        } else if (action == UserAction::Confirm) {
            target.confirmMainMenu();
        }
        return;
    }

    if (page == AppPage::NowPlaying) {
        if (action == UserAction::Back || action == UserAction::Home) {
            target.popPage();
        } else if (isLyricsToggle(event)) {
            target.pushPage(AppPage::Lyrics);
        } else if (isQueueToggle(event)) {
            target.pushPage(AppPage::Queue);
        } else if (action == UserAction::Left) {
            target.stepTrack(-1);
        } else if (action == UserAction::Right || action == UserAction::NextTrack) {
            target.stepTrack(1);
        } else if (action == UserAction::Up) {
            target.toggleShuffle();
        } else if (action == UserAction::Down) {
            target.cycleRepeatMode();
        } else if (action == UserAction::Confirm && !target.nowPlayingConfirmBlocked()) {
            target.togglePlayPause();
        }
        return;
    }

    if (page == AppPage::Lyrics) {
        if (action == UserAction::Back || action == UserAction::Home) {
            target.popPage();
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
            target.popPage();
        } else if (action == UserAction::Left) {
            target.moveEqualizerSelection(-1);
        } else if (action == UserAction::Right) {
            target.moveEqualizerSelection(1);
        } else if (action == UserAction::Up) {
            target.adjustSelectedEqualizerBand(1);
        } else if (action == UserAction::Down) {
            target.adjustSelectedEqualizerBand(-1);
        }
        return;
    }

    if (page == AppPage::Search) {
        if (event.key == InputKey::Character && event.text != '\0') {
            target.appendSearchCharacter(event.text);
            return;
        }
        if (event.key == InputKey::Backspace) {
            target.backspaceSearchQuery();
            return;
        }
    }

    if (target.isBrowseListPage()) {
        if (event.key == InputKey::F4 && (page == AppPage::Playlists || page == AppPage::PlaylistDetail)) {
            target.pushPage(AppPage::PlaylistEditor);
            return;
        }
        if (event.key == InputKey::F2 && page != AppPage::Search) {
            target.pushPage(AppPage::Search);
            return;
        }
        if (event.key == InputKey::F3 && (page == AppPage::Songs || page == AppPage::PlaylistDetail)) {
            target.cycleSongSortModeAndClamp();
            return;
        }
    }

    if (action == UserAction::Back || action == UserAction::Home) {
        target.popPage();
    } else if (action == UserAction::Up) {
        target.moveSelection(-1);
    } else if (action == UserAction::Down) {
        target.moveSelection(1);
    } else if (action == UserAction::Confirm) {
        target.confirmListPage();
    }
}

} // namespace lofibox::app
