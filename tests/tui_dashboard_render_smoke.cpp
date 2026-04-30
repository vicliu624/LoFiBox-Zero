// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "tui/tui_model.h"
#include "tui/tui_renderer.h"

namespace {

lofibox::tui::TuiModel sampleModel()
{
    auto model = lofibox::tui::makeTuiModel({});
    model.runtime_connected = true;
    model.snapshot.playback.status = lofibox::runtime::RuntimePlaybackStatus::Playing;
    model.snapshot.playback.title = "Night Walk";
    model.snapshot.playback.artist = "LoFi Garden";
    model.snapshot.playback.album = "City Lights";
    model.snapshot.playback.source_label = "Local Library";
    model.snapshot.playback.elapsed_seconds = 84.0;
    model.snapshot.playback.duration_seconds = 228;
    model.snapshot.visualization.available = true;
    model.snapshot.visualization.bands = {0.1F, 0.2F, 0.3F, 0.5F, 1.0F, 0.8F, 0.6F, 0.4F, 0.2F, 0.1F};
    model.snapshot.lyrics.available = true;
    model.snapshot.lyrics.current_index = 1;
    model.snapshot.lyrics.visible_lines.push_back(lofibox::runtime::RuntimeLyricLine{0, 10.0, "walking in the rain", "", false});
    model.snapshot.lyrics.visible_lines.push_back(lofibox::runtime::RuntimeLyricLine{1, 20.0, "time moves slowly", "", true});
    model.snapshot.queue.visible_items.push_back(lofibox::runtime::RuntimeQueueItem{1, 0, "Night Walk", "LoFi Garden", "City Lights", "LOCAL", 228, true, true});
    model.snapshot.eq.enabled = true;
    model.snapshot.eq.preset_name = "Focus";
    model.snapshot.remote.source_label = "LOCAL";
    model.snapshot.remote.connection_status = "OK";
    return model;
}

} // namespace

int main()
{
    lofibox::tui::TuiRenderer renderer{};
    auto model = sampleModel();
    auto normal = renderer.render(model, {80, 24}).toString();
    if (normal.find("Night Walk") == std::string::npos
        || normal.find("LoFi Garden") == std::string::npos
        || normal.find("Spectrum") == std::string::npos
        || normal.find("Lyrics") == std::string::npos
        || normal.find("Queue") == std::string::npos
        || normal.find("EQ Focus ON") == std::string::npos) {
        std::cerr << "Expected normal dashboard to match the active playback contract.\n";
        return 1;
    }

    if (normal.find("│├") != std::string::npos || normal.find("├") != std::string::npos) {
        std::cerr << "Expected normal dashboard section dividers to live inside the outer TUI box without nested box corners.\n";
        return 1;
    }

    auto wide = renderer.render(model, {120, 40}).toString();
    if (wide.find("Spectrum") == std::string::npos
        || wide.find("Lyrics") == std::string::npos
        || wide.find("Queue") == std::string::npos
        || wide.find("time moves slowly") == std::string::npos
        || wide.find("31 63 125") == std::string::npos) {
        std::cerr << "Expected wide dashboard to show side-by-side spectrum, lyrics, and queue.\n";
        return 1;
    }

    auto tiny = renderer.render(model, {32, 8}).toString();
    if (tiny.find("Night Walk") == std::string::npos) {
        std::cerr << "Expected 32x8 tiny layout to remain usable.\n";
        return 1;
    }

    model.options.charset = lofibox::tui::TuiCharset::Ascii;
    auto ascii = renderer.render(model, {80, 24}).toString();
    if (ascii.find("+") == std::string::npos || ascii.find("#") == std::string::npos) {
        std::cerr << "Expected ASCII dashboard to use ASCII border/progress glyphs.\n";
        return 1;
    }
    if (ascii.find("\xC2\xB7") != std::string::npos) {
        std::cerr << "Expected ASCII dashboard to avoid Unicode middle-dot separators.\n";
        return 1;
    }

    model.options.charset = lofibox::tui::TuiCharset::Unicode;
    model.snapshot.playback.status = lofibox::runtime::RuntimePlaybackStatus::Paused;
    auto micro = renderer.render(model, {40, 10}).toString();
    if (micro.find("PAUSED") == std::string::npos || micro.find("PLAYING") != std::string::npos) {
        std::cerr << "Expected micro dashboard status to reflect runtime playback truth.\n";
        return 1;
    }

    model.runtime_connected = false;
    model.runtime_error = "Runtime is not reachable";
    auto disconnected = renderer.render(model, {80, 24}).toString();
    if (disconnected.find("runtime disconnected") == std::string::npos) {
        std::cerr << "Expected disconnected runtime error page.\n";
        return 1;
    }
    return 0;
}
