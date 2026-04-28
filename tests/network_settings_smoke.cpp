// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "app/app_runtime_context.h"
#include "app/input_event.h"

int main()
{
    lofibox::app::AppRuntimeContext app{};

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
        std::cerr << "Expected Settings rows to include one Remote Setup entry instead of leaking remote fields.\n";
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
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::RemoteSetup) {
        std::cerr << "Expected Remote Setup to first list supported remote media types.\n";
        return 1;
    }
    auto setup_rows = app.pageModel().rows;
    if (setup_rows.size() < 15U
        || setup_rows[0].first != "Jellyfin"
        || setup_rows[3].first != "Emby"
        || setup_rows[10].first != "NFS"
        || setup_rows[14].first != "DLNA / UPnP") {
        std::cerr << "Expected Settings Remote Setup to list all supported remote media kinds before details.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::RemoteProfileSettings) {
        std::cerr << "Expected selecting a remote media kind to open that kind's concrete settings.\n";
        return 1;
    }
    auto remote_rows = app.pageModel().rows;
    if (remote_rows.size() < 8U
        || remote_rows[3].first != "ADDRESS"
        || remote_rows[4].first != "USER"
        || remote_rows[5].first != "PASSWORD"
        || remote_rows[7].first != "TLS VERIFY") {
        std::cerr << "Expected Settings remote media surface to expose address, user, credential, and TLS fields.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Backspace, "BACK", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Backspace, "BACK", '\0'});
    for (int i = 0; i < 6; ++i) {
        app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Down, "DOWN", '\0'});
    }
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    snapshot = app.snapshot();
    if (snapshot.current_page != lofibox::app::AppPage::About) {
        std::cerr << "Expected About row to open About page.\n";
        return 1;
    }

    return 0;
}
