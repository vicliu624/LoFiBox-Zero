// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/dashboard_tui_page.h"

#include "tui/widgets/tui_eq_view.h"
#include "tui/widgets/tui_lyrics_view.h"
#include "tui/widgets/tui_progress_bar.h"
#include "tui/widgets/tui_queue_view.h"
#include "tui/widgets/tui_spectrum.h"
#include "tui/widgets/tui_status_line.h"
#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {
namespace {

std::string stateGlyph(runtime::RuntimePlaybackStatus status, TuiCharset charset)
{
    if (charset == TuiCharset::Ascii || charset == TuiCharset::Minimal) {
        switch (status) {
        case runtime::RuntimePlaybackStatus::Playing: return "PLAYING";
        case runtime::RuntimePlaybackStatus::Paused: return "PAUSED";
        case runtime::RuntimePlaybackStatus::Empty: return "EMPTY";
        }
    }
    switch (status) {
    case runtime::RuntimePlaybackStatus::Playing: return "▶ PLAYING";
    case runtime::RuntimePlaybackStatus::Paused: return "⏸ PAUSED";
    case runtime::RuntimePlaybackStatus::Empty: return "EMPTY";
    }
    return "UNKNOWN";
}

} // namespace

std::vector<std::string> dashboardPageLines(const TuiModel& model, const TuiLayout& layout)
{
    const int width = layout.size.width;
    std::vector<std::string> lines{};
    if (!model.runtime_connected) {
        lines.push_back("LoFiBox Zero");
        lines.push_back("runtime disconnected");
        lines.push_back(widgets::fitText(model.runtime_error.empty() ? "retrying runtime socket" : model.runtime_error, width));
        lines.push_back("Start LoFiBox device/X11 runtime or run: lofibox runtime status");
        return lines;
    }

    const auto& playback = model.snapshot.playback;
    const std::string title = playback.title.empty() ? "No track loaded" : playback.title;
    const std::string artist = playback.artist.empty() ? "" : " - " + playback.artist;
    lines.push_back(widgets::fitText("LoFiBox Zero · " + layoutKindName(layout.kind), width));
    lines.push_back(widgets::renderStatusLine(model, width));
    lines.push_back(widgets::fitText(stateGlyph(playback.status, model.options.charset) + "  " + title + artist, width));
    const std::string bitrate = playback.bitrate_kbps > 0 ? std::to_string(playback.bitrate_kbps) + "kbps" : std::string{};
    lines.push_back(widgets::fitText(widgets::joinNonEmpty({
        playback.album,
        playback.source_label,
        playback.codec,
        bitrate}, " · "), width));
    lines.push_back(widgets::renderProgressLine(playback.elapsed_seconds, playback.duration_seconds, playback.live, width, model.options.charset));

    if (layout.show_spectrum) {
        lines.push_back("Spectrum");
        lines.push_back(widgets::fitText(widgets::renderSpectrumBars(model.snapshot.visualization, layout.kind == TuiLayoutKind::Wide ? 16 : 10, model.options.charset), width));
    }
    if (layout.show_lyrics) {
        lines.push_back("Lyrics");
        for (const auto& line : widgets::renderLyricsView(model.snapshot.lyrics, layout.kind == TuiLayoutKind::Wide ? 5 : 3, width)) {
            lines.push_back(line);
        }
    }
    if (layout.show_queue) {
        lines.push_back("Queue");
        for (const auto& line : widgets::renderQueueView(model.snapshot.queue, layout.kind == TuiLayoutKind::Wide ? 5 : 3, width)) {
            lines.push_back(line);
        }
    }
    lines.push_back(widgets::fitText("Source: " + model.snapshot.remote.source_label
        + " · " + model.snapshot.remote.connection_status
        + " · EQ " + (model.snapshot.eq.enabled ? "ON" : "OFF"), width));
    if (layout.show_footer) {
        lines.push_back(widgets::renderFooterLine(model, width));
    }
    return lines;
}

} // namespace lofibox::tui::pages
