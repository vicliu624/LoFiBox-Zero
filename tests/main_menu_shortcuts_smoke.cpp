// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <iostream>

#include "app/input_event.h"
#include "app/lofibox_app.h"

namespace fs = std::filesystem;

namespace {

void touchFile(const fs::path& path)
{
    std::ofstream output(path.string(), std::ios::binary);
    output << "test";
}

} // namespace

int main()
{
    const fs::path root = fs::temp_directory_path() / "lofibox_zero_main_menu_shortcuts_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root / "Artist" / "Album");
    touchFile(root / "Artist" / "Album" / "alpha.mp3");
    touchFile(root / "Artist" / "Album" / "beta.mp3");

    lofibox::app::LoFiBoxApp app{{root}};
    app.update();
    app.update();

    auto snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::MainMenu || snapshot.track_count != 2) {
        std::cerr << "Expected Main Menu with two indexed tracks.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::F2, "F2", '\0'});
    snapshot = app.snapshot();
    if (snapshot.playback_status != lofibox::app::PlaybackStatus::Playing || !snapshot.current_track_id) {
        std::cerr << "Expected Main Menu F2 to start or resume playback.\n";
        return 1;
    }

    const int first_id = *snapshot.current_track_id;
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::F3, "F3", '\0'});
    snapshot = app.snapshot();
    if (snapshot.playback_status != lofibox::app::PlaybackStatus::Paused) {
        std::cerr << "Expected Main Menu F3 to pause playback.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::F2, "F2", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::F5, "F5", '\0'});
    snapshot = app.snapshot();
    if (!snapshot.current_track_id || *snapshot.current_track_id == first_id) {
        std::cerr << "Expected Main Menu F5 to advance to the next track.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::F4, "F4", '\0'});
    snapshot = app.snapshot();
    if (!snapshot.current_track_id || *snapshot.current_track_id != first_id) {
        std::cerr << "Expected Main Menu F4 to return to the previous track.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::F6, "F6", '\0'});
    snapshot = app.snapshot();
    if (!snapshot.shuffle_enabled || snapshot.repeat_one || snapshot.repeat_all) {
        std::cerr << "Expected first Main Menu F6 press to enable shuffle mode.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::F6, "F6", '\0'});
    snapshot = app.snapshot();
    if (snapshot.shuffle_enabled || !snapshot.repeat_one) {
        std::cerr << "Expected second Main Menu F6 press to enable single-track repeat.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::F6, "F6", '\0'});
    snapshot = app.snapshot();
    if (snapshot.shuffle_enabled || snapshot.repeat_one || snapshot.repeat_all) {
        std::cerr << "Expected third Main Menu F6 press to return to sequential mode.\n";
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
