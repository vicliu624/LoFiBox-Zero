// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/sources_tui_page.h"

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {

std::vector<std::string> sourcesPageLines(const TuiModel& model, const TuiLayout& layout)
{
    const auto& sources = model.snapshot.sources;
    const auto& remote = model.snapshot.remote;
    return {
        widgets::fitText("Sources", layout.size.width),
        widgets::fitText("configured: " + std::to_string(sources.configured_count), layout.size.width),
        widgets::fitText("active: " + (sources.active_source_label.empty() ? std::string{"none"} : sources.active_source_label), layout.size.width),
        widgets::fitText("profile: " + (sources.active_profile_id.empty() ? std::string{"-"} : sources.active_profile_id), layout.size.width),
        widgets::fitText("connection: " + sources.connection_status, layout.size.width),
        widgets::fitText("url: " + (remote.redacted_url.empty() ? std::string{"-"} : remote.redacted_url), layout.size.width),
        widgets::fitText("buffer: " + remote.buffer_state + " · recovery: " + remote.recovery_action, layout.size.width),
    };
}

} // namespace lofibox::tui::pages

