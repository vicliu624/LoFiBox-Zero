// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>

#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"

namespace {

class ProjectionAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "TUI-PROJECTION-AUDIO"; }
    bool playFile(const std::filesystem::path&, double) override { return true; }
    void stop() override {}
    [[nodiscard]] bool isPlaying() override { return true; }
    [[nodiscard]] bool isFinished() override { return false; }
    [[nodiscard]] lofibox::app::AudioVisualizationFrame visualizationFrame() const override
    {
        lofibox::app::AudioVisualizationFrame frame{};
        frame.available = true;
        frame.bands = {0.0F, 0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F, 0.8F, 0.9F};
        return frame;
    }
};

} // namespace

int main()
{
    auto services = lofibox::app::withNullRuntimeServices();
    services.playback.audio_backend = std::make_shared<ProjectionAudioBackend>();
    lofibox::application::AppServiceHost app_host{services};
    app_host.controllers().library.mutableModel().tracks.push_back(
        lofibox::app::TrackRecord{7, std::filesystem::path{"song.flac"}, "Song", "Artist", "Album"});
    app_host.controllers().library.setSongsContextAll();

    auto& session = app_host.controllers().playback.mutableSession();
    session.status = lofibox::app::PlaybackStatus::Playing;
    session.current_track_id = 7;
    session.audio_active = true;
    session.elapsed_seconds = 12.0;
    session.current_lyrics.synced = "[00:10.00]hello\n[00:12.00]current line\n[00:14.00]next";
    session.current_lyrics.source = "embedded";
    session.visualization_frame.available = true;
    session.visualization_frame.bands = {0.0F, 0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F, 0.8F, 0.9F};

    lofibox::runtime::RuntimeHost runtime_host{app_host.registry()};
    runtime_host.tick(0.25);
    const auto snapshot = runtime_host.client().snapshot();
    if (snapshot.playback.title != "Song" || snapshot.playback.artist != "Artist") {
        std::cerr << "Expected runtime playback projection to include track metadata.\n";
        return 1;
    }
    if (!snapshot.visualization.available || snapshot.visualization.bands[9] < 0.8F) {
        std::cerr << "Expected runtime snapshot to expose visualization bands.\n";
        return 1;
    }
    if (!snapshot.lyrics.available || snapshot.lyrics.current_index != 1 || snapshot.lyrics.visible_lines.empty()) {
        std::cerr << "Expected runtime snapshot to expose visible synced lyrics.\n";
        return 1;
    }
    if (snapshot.library.track_count != 1 || !snapshot.diagnostics.audio_ok) {
        std::cerr << "Expected runtime library and diagnostics projections.\n";
        return 1;
    }
    if (!snapshot.creator.available || !snapshot.creator.waveform_available || snapshot.creator.analysis_source.empty()) {
        std::cerr << "Expected runtime snapshot to expose live creator analysis projection.\n";
        return 1;
    }
    return 0;
}
