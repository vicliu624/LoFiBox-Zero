// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

#include "app/app_debug_snapshot.h"
#include "app/app_page.h"
#include "app/input_event.h"
#include "app/runtime_services.h"
#include "core/canvas.h"
#include "ui/ui_models.h"

namespace lofibox::app {

class AppRuntimeContext;

class LoFiBoxApp {
public:
    explicit LoFiBoxApp(std::vector<std::filesystem::path> media_roots = {}, ui::UiAssets assets = {}, RuntimeServices services = {});
    ~LoFiBoxApp();

    void update();
    void handleInput(const InputEvent& event);
    void render(core::Canvas& canvas) const;

    [[nodiscard]] AppDebugSnapshot snapshot() const;

private:
    std::unique_ptr<AppRuntimeContext> context_{};
};

} // namespace lofibox::app
