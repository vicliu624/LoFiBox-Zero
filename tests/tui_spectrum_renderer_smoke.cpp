// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "tui/widgets/tui_spectrum.h"

int main()
{
    lofibox::runtime::VisualizationRuntimeSnapshot empty{};
    const auto fallback = lofibox::tui::widgets::renderSpectrumBars(empty, 10, lofibox::tui::TuiCharset::Ascii);
    if (fallback.find("unavailable") == std::string::npos) {
        std::cerr << "Expected unavailable spectrum fallback.\n";
        return 1;
    }

    lofibox::runtime::VisualizationRuntimeSnapshot visualization{};
    visualization.available = true;
    visualization.bands = {0.0F, 0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F, 0.8F, 1.0F};
    const auto ascii = lofibox::tui::widgets::renderSpectrumBars(visualization, 10, lofibox::tui::TuiCharset::Ascii);
    const auto unicode = lofibox::tui::widgets::renderSpectrumBars(visualization, 10, lofibox::tui::TuiCharset::Unicode);
    if (ascii.size() < 10 || unicode.find("█") == std::string::npos) {
        std::cerr << "Expected ASCII and Unicode spectrum bars.\n";
        return 1;
    }
    return 0;
}

