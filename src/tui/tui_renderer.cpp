// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/tui_renderer.h"

#include "tui/pages/command_palette_tui_page.h"
#include "tui/pages/creator_tui_page.h"
#include "tui/pages/dashboard_tui_page.h"
#include "tui/pages/diagnostics_tui_page.h"
#include "tui/pages/dsp_tui_page.h"
#include "tui/pages/eq_tui_page.h"
#include "tui/pages/help_tui_page.h"
#include "tui/pages/library_tui_page.h"
#include "tui/pages/lyrics_tui_page.h"
#include "tui/pages/now_playing_tui_page.h"
#include "tui/pages/queue_tui_page.h"
#include "tui/pages/sources_tui_page.h"
#include "tui/pages/spectrum_tui_page.h"
#include "tui/widgets/tui_box.h"
#include "tui/widgets/tui_help_overlay.h"
#include "tui/widgets/tui_text.h"

#include <string>
#include <vector>

namespace lofibox::tui {
namespace {

std::vector<std::string> pageLines(const TuiModel& model, const TuiLayout& layout)
{
    if (model.help_open) {
        return widgets::helpOverlayLines(layout.size.width);
    }
    switch (model.view) {
    case TuiViewMode::Dashboard: return pages::dashboardPageLines(model, layout);
    case TuiViewMode::NowPlaying: return pages::nowPlayingPageLines(model, layout);
    case TuiViewMode::Lyrics: return pages::lyricsPageLines(model, layout);
    case TuiViewMode::Spectrum: return pages::spectrumPageLines(model, layout);
    case TuiViewMode::Queue: return pages::queuePageLines(model, layout);
    case TuiViewMode::Library: return pages::libraryPageLines(model, layout);
    case TuiViewMode::Sources: return pages::sourcesPageLines(model, layout);
    case TuiViewMode::Eq: return pages::eqPageLines(model, layout);
    case TuiViewMode::Dsp: return pages::dspPageLines(model, layout);
    case TuiViewMode::Diagnostics: return pages::diagnosticsPageLines(model, layout);
    case TuiViewMode::Creator: return pages::creatorPageLines(model, layout);
    case TuiViewMode::Help: return pages::helpPageLines(model, layout);
    case TuiViewMode::CommandPalette: return pages::commandPalettePageLines(model, layout);
    }
    return pages::dashboardPageLines(model, layout);
}

} // namespace

TuiFrame TuiRenderer::render(const TuiModel& model, TerminalSize size) const
{
    TuiFrame frame{size.width, size.height};
    const auto layout = classifyTuiLayout(size, model.options);
    if (layout.kind == TuiLayoutKind::TooSmall) {
        frame.writeLine(0, "LoFiBox TUI needs at least 32x8.");
        frame.writeLine(1, "Current: " + std::to_string(size.width) + "x" + std::to_string(size.height) + ".");
        frame.writeLine(2, "Use: lofibox now --porcelain");
        return frame;
    }

    if (model.options.charset != TuiCharset::Minimal && size.width >= 34 && size.height >= 8) {
        widgets::drawBox(frame, 0, 0, size.width, size.height, model.options.charset, " LoFiBox Zero ");
    }
    const int margin = model.options.charset == TuiCharset::Minimal ? 0 : 1;
    const int content_width = size.width - (margin * 2);
    auto lines = pageLines(model, layout);
    int row = margin;
    for (const auto& line : lines) {
        if (row >= size.height - margin) {
            break;
        }
        frame.write(row, margin, widgets::fitText(line, content_width));
        ++row;
    }
    return frame;
}

} // namespace lofibox::tui
