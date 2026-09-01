// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <exception>
#include <iostream>
#include <utility>

#include "app/lofibox_app_runner.h"
#include "cli/direct_cli.h"
#include "cli/runtime_cli.h"
#include "platform/host/legacy_asset_loader.h"
#include "platform/host/media_runtime_capabilities.h"
#include "platform/host/runtime_services_factory.h"
#include "platform/host/single_instance_lock.h"
#include "platform/wayland/widget_presenter.h"
#include "targets/cli_options.h"
#if defined(LOFIBOX_HAVE_TUI)
#include "tui/tui_app.h"
#endif

int main(int argc, char** argv)
{
    try {
        if (lofibox::targets::handleCommonCliOptions(argc, argv, std::cout)) {
            return 0;
        }

#if defined(LOFIBOX_HAVE_TUI)
        if (const auto tui_exit = lofibox::tui::runTuiSubcommandFromArgv(argc, argv, std::cout, std::cerr)) {
            return *tui_exit;
        }
#endif

        if (const auto runtime_cli_exit = lofibox::cli::runRuntimeCliCommand(argc, argv, std::cout, std::cerr)) {
            return *runtime_cli_exit;
        }

        lofibox::platform::host::requireWidgetMediaRuntime();
        auto services = lofibox::platform::host::createHostRuntimeServices();
        if (const auto direct_cli_exit = lofibox::cli::runDirectCliCommand(argc, argv, services, std::cout, std::cerr)) {
            return *direct_cli_exit;
        }

        auto instance_lock = lofibox::platform::host::SingleInstanceLock::acquire();
        if (!instance_lock.acquired()) {
            std::cerr << "Widget startup skipped: " << instance_lock.message() << '\n';
            return 2;
        }

        lofibox::platform::wayland::WidgetPresenter presenter{};
        auto assets = lofibox::platform::host::loadLegacyAssets();
        lofibox::app::runLoFiBoxApp(
            presenter,
            std::move(assets),
            std::move(services),
            std::chrono::milliseconds::zero(),
            lofibox::targets::positionalOpenUris(argc, argv));
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Widget startup failed: " << ex.what() << '\n';
        return 1;
    }
}
