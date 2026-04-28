// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>

#include "app/app_command_executor.h"
#include "app/runtime_services.h"

namespace {

class FakeCommandTarget final : public lofibox::app::AppCommandTarget {
public:
    FakeCommandTarget()
        : services(lofibox::app::withNullRuntimeServices({}))
    {
        controllers.bindServices(services);
    }

    [[nodiscard]] lofibox::app::AppPage currentPage() const noexcept override { return navigation.currentPage(); }

    [[nodiscard]] lofibox::app::AppPageModel pageModel() const override
    {
        const auto page = currentPage();
        return lofibox::app::buildAppPageModel(lofibox::app::AppPageModelInput{
            page,
            controllers.library.titleOverrideForPage(page),
            controllers.library.rowsForPage(page),
            settings,
            true,
            "BUILT-IN"});
    }

    [[nodiscard]] lofibox::application::AppServiceRegistry appServices() noexcept override { return {controllers, services}; }
    lofibox::app::NavigationState& navigationState() noexcept override { return navigation; }
    lofibox::app::EqState& eqState() noexcept override { return eq; }
    int& mainMenuIndex() noexcept override { return menu_index; }
    void closeHelpForCommand() noexcept override { ++close_help_calls; }

    lofibox::app::NavigationState navigation{};
    lofibox::app::RuntimeServices services{};
    lofibox::app::AppControllerSet controllers{};
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

    target.navigation.list_selection.selected = 0;
    lofibox::app::commandMoveSelection(target, -1);
    if (target.navigation.list_selection.selected != 5) {
        std::cerr << "Expected list navigation to wrap upward from first row to last row.\n";
        return 1;
    }
    lofibox::app::commandMoveSelection(target, 1);
    if (target.navigation.list_selection.selected != 0) {
        std::cerr << "Expected list navigation to wrap downward from last row to first row.\n";
        return 1;
    }

    lofibox::app::commandMoveMainMenuSelection(target, -1);
    if (target.menu_index != 0) {
        std::cerr << "Expected main menu selection to wrap left.\n";
        return 1;
    }

    target.navigation.replaceStack({lofibox::app::AppPage::Settings});
    target.navigation.list_selection.selected = 6;
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
    target.eq.preset_name = "FLAT";
    lofibox::app::commandCycleEqualizerPreset(target, 1);
    if (target.eq.preset_name != "BASS BOOST" || target.eq.bands[0] != 6 || target.eq.bands[9] != -2) {
        std::cerr << "Expected EQ preset cycling to apply the next built-in preset.\n";
        return 1;
    }
    lofibox::app::commandAdjustSelectedEqualizerBand(target, 1);
    if (target.eq.preset_name != "CUSTOM") {
        std::cerr << "Expected manual EQ adjustment to mark the preset as CUSTOM.\n";
        return 1;
    }

    auto& model = target.controllers.library.mutableModel();
    model.tracks.push_back(lofibox::app::TrackRecord{7, std::filesystem::path{"song.mp3"}, "Song", "Artist", "Album"});
    target.controllers.library.setSongsContextAll();
    target.navigation.replaceStack({lofibox::app::AppPage::Songs});
    target.navigation.list_selection.selected = 0;
    lofibox::app::commandConfirmListPage(target);
    if (target.currentPage() != lofibox::app::AppPage::NowPlaying || target.controllers.playback.session().current_track_id != 7) {
        std::cerr << "Expected Songs confirm to start track and open Now Playing.\n";
        return 1;
    }

    target.controllers.playback.mutableSession().shuffle_enabled = false;
    target.controllers.playback.mutableSession().repeat_one = false;
    lofibox::app::commandCycleMainMenuPlaybackMode(target);
    if (!target.controllers.playback.session().shuffle_enabled) {
        std::cerr << "Expected menu playback mode cycle to enable shuffle first.\n";
        return 1;
    }
    lofibox::app::commandCycleMainMenuPlaybackMode(target);
    if (target.controllers.playback.session().shuffle_enabled || !target.controllers.playback.session().repeat_all || target.controllers.playback.session().repeat_one) {
        std::cerr << "Expected menu playback mode cycle to move from shuffle to repeat-all.\n";
        return 1;
    }
    lofibox::app::commandCycleMainMenuPlaybackMode(target);
    if (target.controllers.playback.session().shuffle_enabled || target.controllers.playback.session().repeat_all || !target.controllers.playback.session().repeat_one) {
        std::cerr << "Expected menu playback mode cycle to move from repeat-all to repeat-one.\n";
        return 1;
    }

    return 0;
}
