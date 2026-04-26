// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/audio_pipeline_controller.h"

#include <cassert>
#include <filesystem>

namespace {

class FakeAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "FAKE"; }

    bool playFile(const std::filesystem::path&, double start_seconds) override
    {
        started = true;
        last_start_seconds = start_seconds;
        state_value = lofibox::app::AudioPlaybackState::Playing;
        return true;
    }

    void pause() override { state_value = lofibox::app::AudioPlaybackState::Paused; }
    void resume() override { state_value = lofibox::app::AudioPlaybackState::Playing; }
    void stop() override { state_value = lofibox::app::AudioPlaybackState::Finished; }
    [[nodiscard]] bool isPlaying() override { return state_value == lofibox::app::AudioPlaybackState::Playing; }
    [[nodiscard]] bool isFinished() override { return state_value == lofibox::app::AudioPlaybackState::Finished; }
    [[nodiscard]] lofibox::app::AudioPlaybackState state() override { return state_value; }
    [[nodiscard]] lofibox::app::AudioVisualizationFrame visualizationFrame() const override
    {
        lofibox::app::AudioVisualizationFrame frame{};
        frame.bands[0] = 0.75f;
        frame.available = true;
        return frame;
    }

    bool started{false};
    double last_start_seconds{0.0};
    lofibox::app::AudioPlaybackState state_value{lofibox::app::AudioPlaybackState::Idle};
};

} // namespace

int main()
{
    lofibox::app::RuntimeServices services = lofibox::app::withNullRuntimeServices();
    auto backend = std::make_shared<FakeAudioBackend>();
    services.playback.audio_backend = backend;

    lofibox::audio::AudioPipelineController pipeline{};
    pipeline.bind(&services);

    assert(pipeline.startFile("song.mp3", 12.5));
    assert(backend->started);
    assert(backend->last_start_seconds == 12.5);
    assert(pipeline.state() == lofibox::app::AudioPlaybackState::Playing);

    pipeline.pause();
    assert(pipeline.state() == lofibox::app::AudioPlaybackState::Paused);
    pipeline.resume();
    assert(pipeline.state() == lofibox::app::AudioPlaybackState::Playing);

    const auto frame = pipeline.visualizationFrame();
    assert(frame.available);
    assert(frame.bands[0] > 0.7f);

    pipeline.stop();
    assert(pipeline.state() == lofibox::app::AudioPlaybackState::Finished);
    return 0;
}
