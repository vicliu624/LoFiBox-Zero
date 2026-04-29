// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/diagnostics_tui_page.h"

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {

std::vector<std::string> diagnosticsPageLines(const TuiModel& model, const TuiLayout& layout)
{
    const auto& diagnostics = model.snapshot.diagnostics;
    std::vector<std::string> lines{
        widgets::fitText("Diagnostics", layout.size.width),
        widgets::fitText(std::string{"runtime: "} + (diagnostics.runtime_ok ? "OK" : "FAIL"), layout.size.width),
        widgets::fitText(std::string{"audio: "} + (diagnostics.audio_ok ? "OK" : "FAIL") + " · " + diagnostics.audio_backend, layout.size.width),
        widgets::fitText(std::string{"library: "} + (diagnostics.library_ok ? "OK" : "FAIL"), layout.size.width),
        widgets::fitText(std::string{"remote: "} + (diagnostics.remote_ok ? "OK" : "FAIL"), layout.size.width),
        widgets::fitText(std::string{"cache: "} + (diagnostics.cache_ok ? "OK" : "FAIL"), layout.size.width),
    };
    for (const auto& warning : diagnostics.warnings) {
        lines.push_back(widgets::fitText("warning: " + warning, layout.size.width));
    }
    for (const auto& error : diagnostics.errors) {
        lines.push_back(widgets::fitText("error: " + error, layout.size.width));
    }
    return lines;
}

} // namespace lofibox::tui::pages

