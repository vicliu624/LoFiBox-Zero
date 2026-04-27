// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>

#include "app/app_command_executor.h"
#include "app/runtime_services.h"

namespace {

class FakeCommandTarget final : public lofibox::app::AppCommandTarget {
public:
    FakeCommandTarget()
    {
        playback.setServices(lofibox::app::withNullRuntimeServices({}));
    }

    [[nodiscard]] lofibox::app::AppPage currentPage() const noexcept override { return navigation.currentPage(); }

    [[nodiscard]] lofibox::app::AppPageModel pageModel() const override
    {
        const auto page = currentPage();
        return lofibox::app::buildAppPageModel(lofibox::app::AppPageModelInput{
            page,
            library.titleOverrideForPage(page),
            library.rowsForPage(page),
            settings,
            true,
            "BUILT-IN"});
    }

    lofibox::app::NavigationState& navigationState() noexcept override { return navigation; }
    lofibox::app::LibraryController& libraryController() noexcept override { return library; }
    lofibox::app::PlaybackController& playbackController() noexcept override { return playback; }
    lofibox::app::EqState& eqState() noexcept override { return eq; }
    int& mainMenuIndex() noexcept override { return menu_index; }
    void closeHelpForCommand() noexcept override { ++close_help_calls; }

    lofibox::app::NavigationState navigation{};
    lofibox::app::LibraryController library{};
    lofibox::app::PlaybackController playback{};
    lofibox::app::EqState eq{};
    lofibox::app::SettingsState settings{};
    int menu_index{0};
    int close_help_calls{0};
};

} // namespace

int main()
{
    FakeCommandTarget target{};

    target.menu_index = 1;
    lofibox::app::commandConfirmMainMenu(target);
    if (target.currentPage() != lofibox::app::AppPage::MusicIndex || target.close_help_calls != 1) {
        std::cerr << "Expected main menu index 1 to push MusicIndex and close help.\n";
        return 1;
    }

    lofibox::app::commandMoveMainMenuSelection(target, -1);
    if (target.menu_index != 0) {
        std::cerr << "Expected main menu selection to wrap left.\n";
        return 1;
    }

    target.navigation.replaceStack({lofibox::app::AppPage::Settings});
    target.navigation.list_selection.selected = 7;
    lofibox::app::commandConfirmListPage(target);
    if (target.currentPage() != lofibox::app::AppPage::About) {
        std::cerr << "Expected Settings About row to open About.\n";
        return 1;
    }

    target.eq.selected_band = 0;
    target.eq.bands[0] = 12;
    lofibox::app::commandAdjustSelectedEqualizerBand(target, 1);
    if (target.eq.bands[0] != 12) {
        std::cerr << "Expected EQ gain to clamp at +12 dB.\n";
        return 1;
    }
    target.eq.bands[0] = -12;
    lofibox::app::commandAdjustSelectedEqualizerBand(target, -1);
    if (target.eq.bands[0] != -12) {
        std::cerr << "Expected EQ gain to clamp at -12 dB.\n";
        return 1;
    }
    lofibox::app::commandMoveEqualizerSelection(target, 99);
    if (target.eq.selected_band != static_cast<int>(target.eq.bands.size()) - 1) {
        std::cerr << "Expected EQ selected band to clamp to last band.\n";
        return 1;
    }

    auto& model = target.library.mutableModel();
    model.tracks.push_back(lofibox::app::TrackRecord{7, std::filesystem::path{"song.mp3"}, "Song", "Artist", "Album"});
    target.library.setSongsContextAll();
    target.navigation.replaceStack({lofibox::app::AppPage::Songs});
    target.navigation.list_selection.selected = 0;
    lofibox::app::commandConfirmListPage(target);
    if (target.currentPage() != lofibox::app::AppPage::NowPlaying || target.playback.session().current_track_id != 7) {
        std::cerr << "Expected Songs confirm to start track and open Now Playing.\n";
        return 1;
    }

    target.playback.mutableSession().shuffle_enabled = false;
    target.playback.mutableSession().repeat_one = false;
    lofibox::app::commandCycleMainMenuPlaybackMode(target);
    if (!target.playback.session().shuffle_enabled) {
        std::cerr << "Expected menu playback mode cycle to enable shuffle first.\n";
        return 1;
    }
    lofibox::app::commandCycleMainMenuPlaybackMode(target);
    if (target.playback.session().shuffle_enabled || !target.playback.session().repeat_one) {
        std::cerr << "Expected menu playback mode cycle to move from shuffle to repeat-one.\n";
        return 1;
    }

    return 0;
}
