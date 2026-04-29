// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "app/app_debug_snapshot.h"
#include "app/app_page.h"
#include "app/input_event.h"
#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "core/canvas.h"
#include "runtime/runtime_host.h"
#include "ui/ui_models.h"

namespace lofibox::app {

class AppRuntimeContext;
} // namespace lofibox::app

namespace lofibox::app {

class LoFiBoxApp {
public:
    explicit LoFiBoxApp(std::vector<std::filesystem::path> media_roots = {},
                        ui::UiAssets assets = {},
                        RuntimeServices services = {},
                        std::vector<std::string> startup_uris = {},
                        bool enable_external_runtime_transport = false);
    ~LoFiBoxApp();

    void update();
    void handleInput(const InputEvent& event);
    void render(core::Canvas& canvas) const;

    [[nodiscard]] AppDebugSnapshot snapshot() const;
    [[nodiscard]] bool runtimeShutdownRequested() const noexcept;

private:
    using clock = std::chrono::steady_clock;

    RuntimeServices services_{};
    ::lofibox::application::AppServiceHost app_host_;
    ::lofibox::runtime::RuntimeHost runtime_host_;
    std::unique_ptr<AppRuntimeContext> context_{};
    clock::time_point last_runtime_tick_{clock::now()};
};

} // namespace lofibox::app
