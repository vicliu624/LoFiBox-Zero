// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/widgets/tui_spectrum.h"

#include <algorithm>

namespace lofibox::tui::widgets {
namespace {

std::string barGlyph(float value, TuiCharset charset)
{
    const float clamped = std::clamp(value, 0.0F, 1.0F);
    if (charset == TuiCharset::Ascii) {
        return clamped > 0.66F ? "#" : (clamped > 0.33F ? "=" : "-");
    }
    if (charset == TuiCharset::Minimal) {
        return clamped > 0.5F ? "|" : ".";
    }
    static constexpr const char* kGlyphs[] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    const int index = std::clamp(static_cast<int>(clamped * 7.0F), 0, 7);
    return kGlyphs[index];
}

} // namespace

std::string renderSpectrumBars(const runtime::VisualizationRuntimeSnapshot& visualization, int bands, TuiCharset charset)
{
    if (!visualization.available) {
        return charset == TuiCharset::Ascii ? "spectrum unavailable" : "spectrum unavailable";
    }
    const int wanted = std::clamp(bands, 1, 32);
    std::string result{};
    for (int index = 0; index < wanted; ++index) {
        const double mapped = static_cast<double>(index) * (static_cast<double>(visualization.bands.size() - 1U) / static_cast<double>(std::max(1, wanted - 1)));
        const auto source = static_cast<std::size_t>(std::clamp(static_cast<int>(mapped + 0.5), 0, static_cast<int>(visualization.bands.size() - 1U)));
        result += barGlyph(visualization.bands[source], charset);
    }
    return result;
}

std::vector<std::string> renderSpectrumBlock(const runtime::VisualizationRuntimeSnapshot& visualization, int width, int height, TuiCharset charset)
{
    std::vector<std::string> lines{};
    if (height <= 0) {
        return lines;
    }
    if (!visualization.available) {
        lines.push_back("Spectrum unavailable");
        return lines;
    }
    const int bands = std::clamp(width / 2, 5, 32);
    if (height < 4) {
        lines.push_back(renderSpectrumBars(visualization, bands, charset));
        return lines;
    }
    for (int row = height - 1; row >= 0; --row) {
        std::string line{};
        for (int index = 0; index < bands; ++index) {
            const double mapped = static_cast<double>(index) * (static_cast<double>(visualization.bands.size() - 1U) / static_cast<double>(std::max(1, bands - 1)));
            const auto source = static_cast<std::size_t>(std::clamp(static_cast<int>(mapped + 0.5), 0, static_cast<int>(visualization.bands.size() - 1U)));
            const int level = static_cast<int>(std::clamp(visualization.bands[source], 0.0F, 1.0F) * static_cast<float>(height));
            line += level > row ? (charset == TuiCharset::Ascii ? "#" : "█") : " ";
            if (index + 1 < bands) {
                line += ' ';
            }
        }
        lines.push_back(line);
    }
    return lines;
}

} // namespace lofibox::tui::widgets

