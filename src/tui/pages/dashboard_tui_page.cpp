// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/dashboard_tui_page.h"

#include "tui/widgets/tui_lyrics_view.h"
#include "tui/widgets/tui_progress_bar.h"
#include "tui/widgets/tui_queue_view.h"
#include "tui/widgets/tui_spectrum.h"
#include "tui/widgets/tui_status_line.h"
#include "tui/widgets/tui_text.h"

#include <algorithm>

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

std::string repeatGlyph(std::string_view glyph, int count)
{
    std::string result{};
    for (int index = 0; index < count; ++index) {
        result += glyph;
    }
    return result;
}

int cellCount(std::string_view text)
{
    int cells = 0;
    for (std::size_t index = 0; index < text.size();) {
        const unsigned char ch = static_cast<unsigned char>(text[index]);
        std::size_t count = 1;
        if ((ch & 0x80U) != 0U) {
            if ((ch & 0xE0U) == 0xC0U) count = 2;
            else if ((ch & 0xF0U) == 0xE0U) count = 3;
            else if ((ch & 0xF8U) == 0xF0U) count = 4;
        }
        if (index + count > text.size()) {
            break;
        }
        index += count;
        ++cells;
    }
    return cells;
}

std::string padCells(std::string text, int width)
{
    text = widgets::fitText(text, width);
    while (cellCount(text) < width) {
        text.push_back(' ');
    }
    return text;
}

std::string divider(int width, TuiCharset charset, std::string_view title = {})
{
    if (width <= 0) {
        return {};
    }
    const bool ascii = charset == TuiCharset::Ascii || charset == TuiCharset::Minimal;
    const std::string horizontal = ascii ? "-" : "─";
    const std::string label = title.empty() ? std::string{} : " " + std::string(title) + " ";
    const int label_cells = std::min(width, cellCount(label));
    const int left_count = (width - label_cells) / 2;
    const int right_count = width - label_cells - left_count;
    return repeatGlyph(horizontal, left_count) + widgets::fitText(label, label_cells) + repeatGlyph(horizontal, right_count);
}

std::string columnDivider(int width, int first, int second, TuiCharset charset)
{
    const bool ascii = charset == TuiCharset::Ascii || charset == TuiCharset::Minimal;
    const std::string tee = ascii ? "+" : "┼";
    const std::string horizontal = ascii ? "-" : "─";
    const int third = std::max(1, width - first - second - 2);
    return repeatGlyph(horizontal, first)
        + tee
        + repeatGlyph(horizontal, second)
        + tee
        + repeatGlyph(horizontal, third);
}

std::string listSeparator(TuiCharset charset)
{
    return charset == TuiCharset::Ascii || charset == TuiCharset::Minimal ? " - " : " · ";
}

std::string joinColumns(
    std::string left,
    std::string middle,
    std::string right,
    int first,
    int second,
    int third,
    TuiCharset charset)
{
    const std::string separator = charset == TuiCharset::Ascii || charset == TuiCharset::Minimal ? "|" : "│";
    return padCells(std::move(left), first)
        + separator
        + padCells(std::move(middle), second)
        + separator
        + padCells(std::move(right), third);
}

std::string leftRight(std::string left, std::string right, int width)
{
    left = widgets::fitText(left, width);
    right = widgets::fitText(right, width);
    const int gap = width - cellCount(left) - cellCount(right);
    if (gap <= 1) {
        return left;
    }
    return left + repeatGlyph(" ", gap) + right;
}

std::string trackLine(const runtime::PlaybackRuntimeSnapshot& playback, TuiCharset charset, int width)
{
    const std::string title = playback.title.empty() ? "No track loaded" : playback.title;
    const std::string artist = playback.artist.empty() ? std::string{} : " - " + playback.artist;
    const std::string volume = playback.muted ? "Muted" : "Vol " + std::to_string(playback.volume_percent) + "%";
    return leftRight(stateGlyph(playback.status, charset) + "  " + title + artist, volume, width);
}

std::string detailLine(const TuiModel& model, int width)
{
    const auto& playback = model.snapshot.playback;
    const std::string bitrate = playback.bitrate_kbps > 0 ? std::to_string(playback.bitrate_kbps) + "kbps" : std::string{};
    const std::string source = playback.source_label.empty() ? model.snapshot.remote.source_label : playback.source_label;
    const std::string repeat = playback.repeat_one ? "repeat one" : (playback.repeat_all ? "repeat all" : "");
    const std::string shuffle = playback.shuffle_enabled ? "shuffle" : "";
    return widgets::fitText(widgets::joinNonEmpty({
        playback.album,
        source,
        playback.codec,
        bitrate,
        repeat,
        shuffle}, listSeparator(model.options.charset)), width);
}

std::string eqSummary(const TuiModel& model)
{
    return "EQ " + (model.snapshot.eq.preset_name.empty() ? std::string{} : model.snapshot.eq.preset_name + " ")
        + (model.snapshot.eq.enabled ? "ON" : "OFF");
}

std::string sourceSummary(const TuiModel& model)
{
    const auto& remote = model.snapshot.remote;
    const std::string bitrate = remote.bitrate_kbps > 0 ? std::to_string(remote.bitrate_kbps) + "kbps" : std::string{};
    const std::string source = remote.source_label.empty() ? "LOCAL" : remote.source_label;
    return "Source: " + widgets::joinNonEmpty({
        source,
        remote.connection_status,
        remote.buffer_state,
        remote.codec,
        bitrate,
        eqSummary(model)}, listSeparator(model.options.charset));
}

std::vector<std::string> wideDashboard(const TuiModel& model, int width)
{
    std::vector<std::string> lines{};
    const auto& playback = model.snapshot.playback;
    lines.push_back(widgets::renderStatusLine(model, width));
    lines.push_back(trackLine(playback, model.options.charset, width));
    lines.push_back(detailLine(model, width));
    lines.push_back(widgets::renderProgressLine(playback.elapsed_seconds, playback.duration_seconds, playback.live, width, model.options.charset));

    const int first = std::max(18, (width - 4) / 3);
    const int second = first;
    const int third = std::max(18, width - first - second - 4);
    lines.push_back(columnDivider(width, first, second, model.options.charset));

    std::vector<std::string> spectrum_rows{
        "Spectrum",
        widgets::renderSpectrumBars(model.snapshot.visualization, 16, model.options.charset),
        "31 63 125 250 500 1k 2k 4k 8k 16k"};
    auto lyrics_rows = widgets::renderLyricsView(model.snapshot.lyrics, 4, second);
    lyrics_rows.insert(lyrics_rows.begin(), "Lyrics");
    auto queue_rows = widgets::renderQueueView(model.snapshot.queue, 4, third);
    queue_rows.insert(queue_rows.begin(), "Queue");
    const int rows = std::max({static_cast<int>(spectrum_rows.size()), static_cast<int>(lyrics_rows.size()), static_cast<int>(queue_rows.size())});
    for (int row = 0; row < rows; ++row) {
        lines.push_back(joinColumns(
            row < static_cast<int>(spectrum_rows.size()) ? spectrum_rows[static_cast<std::size_t>(row)] : "",
            row < static_cast<int>(lyrics_rows.size()) ? lyrics_rows[static_cast<std::size_t>(row)] : "",
            row < static_cast<int>(queue_rows.size()) ? queue_rows[static_cast<std::size_t>(row)] : "",
            first,
            second,
            third,
            model.options.charset));
    }
    lines.push_back(divider(width, model.options.charset));
    lines.push_back(widgets::fitText(sourceSummary(model), width));
    lines.push_back(widgets::renderFooterLine(model, width));
    return lines;
}

std::vector<std::string> normalDashboard(const TuiModel& model, int width)
{
    std::vector<std::string> lines{};
    const auto& playback = model.snapshot.playback;
    lines.push_back(widgets::renderStatusLine(model, width));
    lines.push_back(trackLine(playback, model.options.charset, width));
    lines.push_back(leftRight(
        widgets::renderProgressLine(playback.elapsed_seconds, playback.duration_seconds, playback.live, std::max(20, width - 16), model.options.charset),
        eqSummary(model),
        width));
    lines.push_back(divider(width, model.options.charset, "Spectrum"));
    lines.push_back(widgets::fitText(widgets::renderSpectrumBars(model.snapshot.visualization, 10, model.options.charset), width));
    lines.push_back(divider(width, model.options.charset, "Lyrics"));
    for (const auto& line : widgets::renderLyricsView(model.snapshot.lyrics, 3, width)) {
        lines.push_back(line);
    }
    lines.push_back(divider(width, model.options.charset, "Queue"));
    std::string summary{"next: "};
    bool first = true;
    for (const auto& item : model.snapshot.queue.visible_items) {
        if (item.active) {
            continue;
        }
        if (!first) {
            summary += " / ";
        }
        summary += item.title.empty() ? "Unknown" : item.title;
        first = false;
        if (cellCount(summary) > width - 12) {
            break;
        }
    }
    lines.push_back(first ? std::string{"next: none"} : widgets::fitText(summary, width));
    lines.push_back(widgets::fitText(sourceSummary(model), width));
    lines.push_back(widgets::renderFooterLine(model, width));
    return lines;
}

std::vector<std::string> compactDashboard(const TuiModel& model, int width)
{
    const auto& playback = model.snapshot.playback;
    std::vector<std::string> lines{};
    lines.push_back(leftRight("LoFiBox Zero  " + stateGlyph(playback.status, model.options.charset), "", width));
    lines.push_back(widgets::fitText((playback.title.empty() ? "No track loaded" : playback.title)
        + (playback.artist.empty() ? std::string{} : " - " + playback.artist), width));
    lines.push_back(leftRight(
        widgets::renderProgressLine(playback.elapsed_seconds, playback.duration_seconds, playback.live, std::max(12, width - 10), model.options.charset),
        playback.muted ? "Muted" : "Vol " + std::to_string(playback.volume_percent) + "%",
        width));
    lines.push_back("");
    lines.push_back(widgets::fitText(widgets::renderSpectrumBars(model.snapshot.visualization, 10, model.options.charset), width));
    lines.push_back("");
    const auto lyrics = widgets::renderLyricsView(model.snapshot.lyrics, 1, width);
    lines.push_back(lyrics.empty() ? std::string{"Lyrics unavailable"} : lyrics.front());
    const auto queue = widgets::renderQueueView(model.snapshot.queue, 2, width);
    lines.push_back(queue.size() > 1 ? widgets::fitText("next: " + queue[1], width) : "next: none");
    lines.push_back(widgets::renderFooterLine(model, width));
    return lines;
}

std::vector<std::string> microDashboard(const TuiModel& model, int width)
{
    const auto& playback = model.snapshot.playback;
    const std::string title = playback.title.empty() ? "No track loaded" : playback.title;
    std::vector<std::string> lines{};
    lines.push_back(widgets::fitText(stateGlyph(playback.status, model.options.charset) + "  " + title, width));
    lines.push_back(leftRight(
        widgets::formatDuration(playback.elapsed_seconds) + "/" + widgets::formatDuration(playback.duration_seconds),
        playback.muted ? "Muted" : "Vol" + std::to_string(playback.volume_percent),
        width));
    lines.push_back(widgets::fitText(widgets::renderSpectrumBars(model.snapshot.visualization, 10, model.options.charset), width));
    const auto lyrics = widgets::renderLyricsView(model.snapshot.lyrics, 1, width);
    lines.push_back(lyrics.empty() ? std::string{"Lyrics unavailable"} : lyrics.front());
    lines.push_back(model.options.charset == TuiCharset::Ascii || model.options.charset == TuiCharset::Minimal
        ? "n next - q quit"
        : "n next · q quit");
    return lines;
}

} // namespace

std::vector<std::string> dashboardPageLines(const TuiModel& model, const TuiLayout& layout)
{
    const int width = std::max(1, layout.size.width - 2);
    std::vector<std::string> lines{};
    if (!model.runtime_connected) {
        lines.push_back("LoFiBox Zero");
        lines.push_back("runtime disconnected");
        lines.push_back(widgets::fitText(model.runtime_error.empty() ? "retrying runtime socket" : model.runtime_error, width));
        lines.push_back("Start LoFiBox device/X11 runtime or run: lofibox runtime status");
        return lines;
    }

    switch (layout.kind) {
    case TuiLayoutKind::Wide:
        return wideDashboard(model, width);
    case TuiLayoutKind::Normal:
        return normalDashboard(model, width);
    case TuiLayoutKind::Compact:
        return compactDashboard(model, width);
    case TuiLayoutKind::Micro:
        return microDashboard(model, width);
    case TuiLayoutKind::Tiny: {
        const auto& playback = model.snapshot.playback;
        const std::string title = playback.title.empty() ? "No track loaded" : playback.title;
        lines.push_back(widgets::fitText(stateGlyph(playback.status, model.options.charset) + "  " + title, width));
        lines.push_back(widgets::formatDuration(playback.elapsed_seconds) + "/" + widgets::formatDuration(playback.duration_seconds));
        lines.push_back(widgets::fitText(widgets::renderSpectrumBars(model.snapshot.visualization, 5, model.options.charset), width));
        lines.push_back("q quit");
        return lines;
    }
    case TuiLayoutKind::TooSmall:
        break;
    }
    return normalDashboard(model, width);
}

} // namespace lofibox::tui::pages
