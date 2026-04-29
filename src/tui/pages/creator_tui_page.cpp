// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/creator_tui_page.h"

#include <algorithm>
#include <sstream>
#include <string_view>

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {
namespace {

std::string formatDouble(double value, int precision = 1)
{
    std::ostringstream out{};
    out.setf(std::ios::fixed);
    out.precision(precision);
    out << value;
    return out.str();
}

std::string waveformLine(const runtime::CreatorRuntimeSnapshot& creator)
{
    if (!creator.waveform_available || creator.waveform_points.empty()) {
        return "waveform: waiting";
    }
    std::string line{"waveform: "};
    constexpr std::string_view levels{"_-=+*#"};
    const auto start = creator.waveform_points.size() > 32 ? creator.waveform_points.end() - 32 : creator.waveform_points.begin();
    for (auto it = start; it != creator.waveform_points.end(); ++it) {
        const auto normalized = std::clamp((*it + 1.0F) * 0.5F, 0.0F, 1.0F);
        const auto index = static_cast<std::size_t>(std::clamp(static_cast<int>(normalized * static_cast<float>(levels.size() - 1)), 0, static_cast<int>(levels.size() - 1)));
        line.push_back(levels[index]);
    }
    return line;
}

std::string beatLine(const runtime::CreatorRuntimeSnapshot& creator)
{
    if (!creator.beat_grid_available || creator.beat_grid_seconds.empty()) {
        return "beat grid: waiting";
    }
    std::ostringstream out{};
    out << "beat grid:";
    for (const auto beat : creator.beat_grid_seconds) {
        out << ' ' << formatDouble(beat, 2);
    }
    return out.str();
}

} // namespace

std::vector<std::string> creatorPageLines(const TuiModel& model, const TuiLayout& layout)
{
    const auto& creator = model.snapshot.creator;
    return {
        widgets::fitText("Creator", layout.size.width),
        widgets::fitText(creator.status_message, layout.size.width),
        widgets::fitText("source: " + (creator.analysis_source.empty() ? std::string{"runtime"} : creator.analysis_source) + " · " + creator.confidence, layout.size.width),
        widgets::fitText("BPM: " + formatDouble(creator.bpm), layout.size.width),
        widgets::fitText("Key: " + (creator.key.empty() ? std::string{"unknown"} : creator.key), layout.size.width),
        widgets::fitText("LUFS: " + formatDouble(creator.lufs), layout.size.width),
        widgets::fitText("Dynamic Range: " + formatDouble(creator.dynamic_range) + " dB", layout.size.width),
        widgets::fitText(waveformLine(creator), layout.size.width),
        widgets::fitText(beatLine(creator), layout.size.width),
        widgets::fitText("stems: " + creator.stem_status, layout.size.width),
        widgets::fitText(creator.section_markers.empty() ? std::string{"sections: waiting"} : "sections: " + creator.section_markers.front(), layout.size.width),
    };
}

} // namespace lofibox::tui::pages
