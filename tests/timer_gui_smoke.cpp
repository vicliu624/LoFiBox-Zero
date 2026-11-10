// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <cassert>
#include "app/app_runtime_context.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"

int main()
{
    using namespace lofibox;
    for (const bool framebuffer : {false, true}) {
        auto services = app::withNullRuntimeServices();
        services.ui.framebuffer_backlight = framebuffer;
        application::AppServiceHost host{services};
        runtime::RuntimeHost runtime{host.registry()};
        app::AppRuntimeContext context{{}, {}, {}, host, runtime.client()};
        context.openSettingsPage();
        const auto row = [&](const char* name) {
            const auto rows = context.pageModel().rows;
            for (std::size_t i = 0; i < rows.size(); ++i) if (rows[i].first == name) return static_cast<int>(i);
            return -1;
        };
        const auto settings = [&] { return runtime.client().query(runtime::RuntimeQuery{
            runtime::RuntimeQueryKind::SettingsSnapshot}).settings; };
        assert((row("SCREEN OFF") >= 0) == framebuffer);
        context.navigationState().list_selection.selected = row("SLEEP TIMER");
        context.handleInput(app::InputEvent{app::InputKey::Enter, "ENTER"});
        assert(settings().sleep_timer == "15M");
        context.handleInput(app::InputEvent{app::InputKey::Character, "6", '6'});
        assert(settings().sleep_timer == "30M");
        if (framebuffer) {
            context.navigationState().list_selection.selected = row("SCREEN OFF");
            context.handleInput(app::InputEvent{app::InputKey::Character, "6", '6'});
            assert(settings().screen_timeout == "30S" && settings().sleep_timer == "30M");
        }
        auto result = runtime.client().dispatch(runtime::RuntimeCommand{runtime::RuntimeCommandKind::SettingsApplyLive,
            runtime::RuntimeCommandPayload::settingsApplyLive({}, {}, "OFF", framebuffer ? "1M" : "OFF")});
        assert(result.applied);
        assert(context.pageModel().rows[static_cast<std::size_t>(row("SLEEP TIMER"))].second == "OFF");
        result = runtime.client().dispatch(runtime::RuntimeCommand{runtime::RuntimeCommandKind::SettingsApplyLive,
            runtime::RuntimeCommandPayload::settingsApplyLive({}, {}, "10M", "invalid")});
        assert(!result.applied && settings().sleep_timer == "OFF");
        if (!framebuffer) {
            result = runtime.client().dispatch(runtime::RuntimeCommand{runtime::RuntimeCommandKind::SettingsApplyLive,
                runtime::RuntimeCommandPayload::settingsApplyLive({}, {}, {}, "30S")});
            assert(!result.applied && settings().screen_timeout == "OFF");
        }
    }
}
