// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"

int main()
{
    auto services = lofibox::app::withNullRuntimeServices();
    lofibox::application::AppServiceHost app_host{services};
    lofibox::runtime::RuntimeHost runtime_host{app_host.registry()};

    const auto result = runtime_host.client().dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::SettingsApplyLive,
        lofibox::runtime::RuntimeCommandPayload::settingsApplyLive("ALSA", "LAN_ONLY", "15M"),
        lofibox::runtime::CommandOrigin::DirectTest,
        "settings-apply"});
    if (!result.accepted || !result.applied || result.code != "SETTINGS_APPLY_LIVE") {
        std::cerr << "Expected SettingsApplyLive command to be implemented.\n";
        return 1;
    }

    const auto snapshot = runtime_host.client().query(lofibox::runtime::RuntimeQuery{
        lofibox::runtime::RuntimeQueryKind::SettingsSnapshot,
        lofibox::runtime::CommandOrigin::DirectTest,
        "settings-snapshot"});
    if (snapshot.settings.output_mode != "ALSA"
        || snapshot.settings.network_policy != "LAN_ONLY"
        || snapshot.settings.sleep_timer != "15M") {
        std::cerr << "Expected live settings snapshot to reflect SettingsApplyLive payload.\n";
        return 1;
    }

    return 0;
}
