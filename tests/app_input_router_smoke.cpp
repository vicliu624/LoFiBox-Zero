// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "app/app_input_router.h"

namespace {

class FakeTarget final : public lofibox::app::AppInputTarget {
public:
    [[nodiscard]] lofibox::app::AppPage currentPage() const noexcept override { return page; }
    [[nodiscard]] bool helpOpen() const noexcept override { return help_open; }
    [[nodiscard]] bool isBrowseListPage() const noexcept override { return browse_list_page; }
    [[nodiscard]] bool nowPlayingConfirmBlocked() const override { return now_playing_confirm_blocked; }

    void toggleHelpForCurrentPage() override { ++toggle_help_calls; help_open = !help_open; }
    void closeHelp() override { ++close_help_calls; help_open = false; }
    void pushPage(lofibox::app::AppPage next_page) override { ++push_page_calls; page = next_page; }
    void popPage() override { ++pop_page_calls; page = lofibox::app::AppPage::MainMenu; }

    void playFromMenu() override { ++play_from_menu_calls; }
    void pausePlayback() override { ++pause_calls; }
    void stepTrack(int delta) override { last_step_delta = delta; ++step_track_calls; }
    void cycleMainMenuPlaybackMode() override { ++cycle_menu_mode_calls; }
    void moveMainMenuSelection(int delta) override { main_menu_delta += delta; }
    void resetMainMenuSelection() override { ++reset_main_menu_calls; }
    void confirmMainMenu() override { ++confirm_main_menu_calls; }

    void toggleShuffle() override { ++toggle_shuffle_calls; }
    void cycleRepeatMode() override { ++cycle_repeat_calls; }
    void togglePlayPause() override { ++toggle_play_pause_calls; }

    void moveEqualizerSelection(int delta) override { eq_selection_delta += delta; }
    void adjustSelectedEqualizerBand(int delta) override { eq_band_delta += delta; }

    void cycleSongSortModeAndClamp() override { ++cycle_sort_calls; }
    void moveSelection(int delta) override { list_delta += delta; }
    void confirmListPage() override { ++confirm_list_calls; }

    lofibox::app::AppPage page{lofibox::app::AppPage::MainMenu};
    bool help_open{false};
    bool browse_list_page{false};
    bool now_playing_confirm_blocked{false};
    int toggle_help_calls{0};
    int close_help_calls{0};
    int push_page_calls{0};
    int pop_page_calls{0};
    int play_from_menu_calls{0};
    int pause_calls{0};
    int step_track_calls{0};
    int last_step_delta{0};
    int cycle_menu_mode_calls{0};
    int main_menu_delta{0};
    int reset_main_menu_calls{0};
    int confirm_main_menu_calls{0};
    int toggle_shuffle_calls{0};
    int cycle_repeat_calls{0};
    int toggle_play_pause_calls{0};
    int eq_selection_delta{0};
    int eq_band_delta{0};
    int cycle_sort_calls{0};
    int list_delta{0};
    int confirm_list_calls{0};
};

[[nodiscard]] lofibox::app::InputEvent key(lofibox::app::InputKey input_key)
{
    return lofibox::app::InputEvent{input_key, "", '\0'};
}

[[nodiscard]] lofibox::app::InputEvent character(char value)
{
    return lofibox::app::InputEvent{lofibox::app::InputKey::Character, "", value};
}

} // namespace

int main()
{
    FakeTarget target{};

    lofibox::app::routeInput(target, key(lofibox::app::InputKey::F1));
    if (!target.help_open || target.toggle_help_calls != 1) {
        std::cerr << "Expected F1 to toggle page help.\n";
        return 1;
    }

    lofibox::app::routeInput(target, key(lofibox::app::InputKey::Enter));
    if (target.help_open || target.close_help_calls != 1) {
        std::cerr << "Expected Enter to close open help.\n";
        return 1;
    }

    lofibox::app::routeInput(target, key(lofibox::app::InputKey::F2));
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::F3));
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::F4));
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::Right));
    if (target.play_from_menu_calls != 1 || target.pause_calls != 1 || target.last_step_delta != -1 || target.main_menu_delta != 1) {
        std::cerr << "Expected main menu shortcuts and navigation to route to menu commands.\n";
        return 1;
    }

    target.page = lofibox::app::AppPage::NowPlaying;
    lofibox::app::routeInput(target, character('L'));
    if (target.page != lofibox::app::AppPage::Lyrics || target.push_page_calls != 1) {
        std::cerr << "Expected Now Playing L to push lyrics page.\n";
        return 1;
    }

    lofibox::app::routeInput(target, key(lofibox::app::InputKey::Left));
    if (target.last_step_delta != -1) {
        std::cerr << "Expected lyrics page transport controls to route.\n";
        return 1;
    }

    target.page = lofibox::app::AppPage::NowPlaying;
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::Down));
    if (target.cycle_repeat_calls != 1) {
        std::cerr << "Expected Now Playing repeat control to route.\n";
        return 1;
    }

    target.page = lofibox::app::AppPage::Equalizer;
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::Right));
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::Up));
    if (target.eq_selection_delta != 1 || target.eq_band_delta != 1) {
        std::cerr << "Expected equalizer page controls to route.\n";
        return 1;
    }

    target.page = lofibox::app::AppPage::Songs;
    target.browse_list_page = true;
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::F3));
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::Down));
    lofibox::app::routeInput(target, key(lofibox::app::InputKey::Enter));
    if (target.cycle_sort_calls != 1 || target.list_delta != 1 || target.confirm_list_calls != 1) {
        std::cerr << "Expected browse list controls to route.\n";
        return 1;
    }

    return 0;
}
