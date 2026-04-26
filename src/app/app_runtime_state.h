// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <filesystem>
#include <vector>

#include "app/app_page.h"
#include "app/remote_media_services.h"
#include "app/app_state.h"
#include "app/navigation_state.h"
#include "ui/ui_models.h"

namespace lofibox::app {

struct AppRuntimeState {
    using clock = std::chrono::steady_clock;

    std::vector<std::filesystem::path> media_roots{};
    ui::UiAssets ui_assets{};
    clock::time_point boot_started{clock::now()};
    NavigationState navigation{};
    int main_menu_index{1};
    SettingsState settings{};
    NetworkState network{};
    MetadataServiceState metadata_service{};
    EqState eq{};
    std::vector<RemoteServerProfile> remote_profiles{};
    clock::time_point last_update{clock::now()};
    clock::time_point last_status_refresh{};
    clock::time_point now_playing_confirm_blocked_until{};
    bool help_open{false};
    AppPage help_page{AppPage::MainMenu};
};

} // namespace lofibox::app
