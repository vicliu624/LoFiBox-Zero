// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include "tui/tui_input_router.h"
#include "tui/tui_command_palette.h"
#include "runtime/runtime_command.h"

int main()
{
    using namespace lofibox;
    const auto sleep = tui::commandPaletteRuntimeCommand("sleep 30m");
    assert(sleep && sleep->kind == runtime::RuntimeCommandKind::SettingsApplyLive);
    const auto* sleep_payload = sleep->payload.get<runtime::SettingsApplyLivePayload>();
    assert(sleep_payload && sleep_payload->sleep_timer == "30M" && sleep_payload->screen_timeout.empty());
    const auto screen = tui::commandPaletteRuntimeCommand("screen 1m");
    const auto* screen_payload = screen ? screen->payload.get<runtime::SettingsApplyLivePayload>() : nullptr;
    assert(screen_payload && screen_payload->sleep_timer.empty() && screen_payload->screen_timeout == "1M");
    assert(tui::commandPaletteRuntimeCommand("sleep off")->payload.get<runtime::SettingsApplyLivePayload>()->sleep_timer == "OFF");
    for (const auto text : {"sleep 999H", "screen -1m", "sleep 15m; shutdown", "!sleep 1m", "screen on"})
        assert(!tui::commandPaletteRuntimeCommand(text));
}
