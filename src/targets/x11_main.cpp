// SPDX-License-Identifier: GPL-3.0-or-later

#include <exception>
#include <chrono>
#include <iostream>
#include <utility>

#include "app/lofibox_app_runner.h"
#include "platform/host/legacy_asset_loader.h"
#include "platform/host/runtime_services_factory.h"
#include "platform/host/single_instance_lock.h"
#include "platform/x11/x11_presenter.h"
#include "targets/cli_options.h"

int main(int argc, char** argv)
{
    try {
        if (lofibox::targets::handleCommonCliOptions(argc, argv, std::cout)) {
            return 0;
        }

        auto instance_lock = lofibox::platform::host::SingleInstanceLock::acquire();
        if (!instance_lock.acquired()) {
            std::cerr << "X11 startup skipped: " << instance_lock.message() << '\n';
            return 2;
        }
        lofibox::platform::x11::X11Presenter presenter{};
        auto assets = lofibox::platform::host::loadLegacyAssets();
        auto services = lofibox::platform::host::createHostRuntimeServices();
        lofibox::app::runLoFiBoxApp(
            presenter,
            std::move(assets),
            std::move(services),
            std::chrono::milliseconds::zero(),
            lofibox::targets::positionalOpenUris(argc, argv));
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "X11 startup failed: " << ex.what() << '\n';
        return 1;
    }
}
