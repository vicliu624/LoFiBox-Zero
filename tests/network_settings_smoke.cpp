// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "app/input_event.h"
#include "app/lofibox_app.h"

int main()
{
    lofibox::app::LoFiBoxApp app{};

    app.update();
    app.update();

    auto snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::MainMenu) {
        std::cerr << "Expected Boot to transition to Main Menu.\n";
        return 1;
    }

    for (int i = 0; i < 4; ++i) {
        app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Right, "RIGHT", '\0'});
    }
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});

    snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::Settings) {
        std::cerr << "Expected Main Menu to enter Settings.\n";
        return 1;
    }

    if (snapshot.visible_count != 7) {
        std::cerr << "Expected seven Settings rows including Network and Metadata.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::Settings) {
        std::cerr << "Expected Network row to remain read-only.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Down, "DOWN", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::Settings) {
        std::cerr << "Expected Metadata Service row to remain read-only.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Down, "DOWN", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::Settings) {
        std::cerr << "Expected Sleep Timer to stay on Settings page.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Down, "DOWN", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Down, "DOWN", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Down, "DOWN", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Down, "DOWN", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::About) {
        std::cerr << "Expected About row to open About page.\n";
        return 1;
    }

    return 0;
}
