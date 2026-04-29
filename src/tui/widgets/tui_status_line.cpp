// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/widgets/tui_status_line.h"

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::widgets {

std::string renderStatusLine(const TuiModel& model, int width)
{
    const std::string runtime = model.runtime_connected ? "runtime: connected" : "runtime: disconnected";
    const std::string view = "mode: " + tuiViewModeName(model.view);
    const std::string output = model.snapshot.diagnostics.audio_backend.empty()
        ? "output: unknown"
        : "output: " + model.snapshot.diagnostics.audio_backend;
    return fitText(joinNonEmpty({runtime, output, view}, " · "), width);
}

std::string renderFooterLine(const TuiModel&, int width)
{
    return fitText("q quit · space play/pause · n next · p prev · l lyrics · v spectrum · ? help", width);
}

} // namespace lofibox::tui::widgets

