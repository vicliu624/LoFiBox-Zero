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

    const auto shutdown = runtime_host.client().dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::RuntimeShutdown,
        {},
        lofibox::runtime::CommandOrigin::DirectTest,
        "shutdown"});
    if (!shutdown.accepted || !shutdown.applied || !runtime_host.shutdownRequested()) {
        std::cerr << "Expected RuntimeShutdown command to set the host shutdown request.\n";
        return 1;
    }

    const auto reload = runtime_host.client().dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::RuntimeReload,
        {},
        lofibox::runtime::CommandOrigin::DirectTest,
        "reload"});
    if (!reload.accepted || !reload.applied || !runtime_host.reloadRequested()) {
        std::cerr << "Expected RuntimeReload command to set the host reload request.\n";
        return 1;
    }

    return 0;
}
