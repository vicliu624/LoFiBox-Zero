// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include "app/input_event.h"
#include "app/lofibox_app.h"
#include "app/runtime_services.h"
#include "audio/groove/sample_buffer.h"
#include "audio/groove/wav_exporter.h"
#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace fs = std::filesystem;

namespace {

class FakeAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "FAKE"; }
    bool playFile(const std::filesystem::path& path, double) override
    {
        last_file = path;
        ++play_file_calls;
        playing = true;
        paused = false;
        return true;
    }
    bool playUri(const std::string&, double) override
    {
        ++play_uri_calls;
        playing = true;
        paused = false;
        return true;
    }
    void stop() override
    {
        ++stop_calls;
        playing = false;
        paused = false;
    }
    void pause() override
    {
        ++pause_calls;
        paused = true;
    }
    void resume() override
    {
        ++resume_calls;
        playing = true;
        paused = false;
    }
    [[nodiscard]] bool isPlaying() override { return playing && !paused; }
    [[nodiscard]] bool isFinished() override { return false; }

    int play_file_calls{0};
    int play_uri_calls{0};
    int pause_calls{0};
    int resume_calls{0};
    int stop_calls{0};
    fs::path last_file{};

private:
    bool playing{false};
    bool paused{false};
};

void setEnvPath(const char* name, const fs::path& path)
{
#if defined(_WIN32)
    _putenv_s(name, path.string().c_str());
#else
    setenv(name, path.string().c_str(), 1);
#endif
}

void writeTestWav(const fs::path& path)
{
    auto buffer = lofibox::audio::groove::makeSilentSampleBuffer(48000, 1, 2.0);
    for (std::size_t frame = 0; frame < buffer.frameCount(); ++frame) {
        const float pulse = frame % 2400U < 1200U ? 0.25f : -0.25f;
        buffer.samples[frame] = pulse;
    }
    lofibox::audio::groove::WavExporter exporter{};
    const auto result = exporter.writePcm16(path, buffer, false);
    if (!result.ok) {
        throw std::runtime_error(result.errorMessage);
    }
}

void press(lofibox::app::LoFiBoxApp& app, lofibox::app::InputKey key, const char* label = "")
{
    app.handleInput(lofibox::app::InputEvent{key, label, '\0'});
}

void pressChar(lofibox::app::LoFiBoxApp& app, char ch)
{
    app.handleInput(lofibox::app::makeCharacterInput(ch));
}

int nonBackgroundPixels(const lofibox::core::Canvas& canvas)
{
    const auto background = lofibox::ui::defaultTheme().palette.background;
    int count = 0;
    for (const auto& pixel : canvas.pixels()) {
        if (pixel != background) {
            ++count;
        }
    }
    return count;
}

} // namespace

int main()
{
    const fs::path root = fs::temp_directory_path() / "lofibox_zero_pocket_groove_runtime_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root / "Artist" / "Album");
    fs::create_directories(root / "xdg-data");
    fs::create_directories(root / "xdg-cache");
    fs::create_directories(root / "xdg-config");
    setEnvPath("XDG_DATA_HOME", root / "xdg-data");
    setEnvPath("XDG_CACHE_HOME", root / "xdg-cache");
    setEnvPath("XDG_CONFIG_HOME", root / "xdg-config");
    setEnvPath("HOME", root / "home");
    writeTestWav(root / "Artist" / "Album" / "alpha.wav");

    auto backend = std::make_shared<FakeAudioBackend>();
    auto services = lofibox::app::withNullRuntimeServices();
    services.playback.audio_backend = backend;
    lofibox::app::LoFiBoxApp app{{root}, {}, std::move(services)};
    for (int tick = 0; tick < 500; ++tick) {
        app.update();
        if (app.snapshot().library_ready) {
            break;
        }
    }
    if (!app.snapshot().library_ready || app.snapshot().track_count != 1) {
        std::cerr << "Expected one WAV track to be indexed for groove capture.\n";
        return 1;
    }

    press(app, lofibox::app::InputKey::F2, "F2");
    if (app.snapshot().playback_status != lofibox::app::PlaybackStatus::Playing || backend->play_file_calls == 0) {
        std::cerr << "Expected F2 to start current playback before entering Groove.\n";
        return 1;
    }

    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Enter, "OK");
    if (app.snapshot().current_page != lofibox::app::AppPage::PocketGroove || backend->pause_calls == 0) {
        std::cerr << "Expected Main Menu Groove entry to open PocketGroove and pause playback.\n";
        return 1;
    }

    const int play_calls_before_groove = backend->play_file_calls;
    press(app, lofibox::app::InputKey::F2, "F2");
    if (backend->play_file_calls <= play_calls_before_groove || backend->last_file.filename() != "preview.wav") {
        std::cerr << "Expected Pocket Groove F2 to render and play a groove preview WAV.\n";
        return 1;
    }

    lofibox::core::Canvas canvas{320, 170};
    app.render(canvas);
    if (nonBackgroundPixels(canvas) < 1000) {
        std::cerr << "Expected Pocket Groove AppRenderer branch to draw the groove view.\n";
        return 1;
    }

    press(app, lofibox::app::InputKey::Backspace, "BACK");
    if (app.snapshot().current_page != lofibox::app::AppPage::MainMenu || backend->stop_calls == 0) {
        std::cerr << "Expected Back from Pocket Groove to exit to Main Menu and stop groove playback.\n";
        return 1;
    }

    press(app, lofibox::app::InputKey::Home, "HOME");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Enter, "OK");
    if (app.snapshot().current_page != lofibox::app::AppPage::NowPlaying) {
        std::cerr << "Expected menu NOW entry to open Now Playing.\n";
        return 1;
    }
    pressChar(app, 'G');
    if (app.snapshot().current_page != lofibox::app::AppPage::PocketGroove) {
        std::cerr << "Expected Now Playing G to open Pocket Groove capture.\n";
        return 1;
    }
    press(app, lofibox::app::InputKey::Enter, "OK");
    const auto sample_dir = root / "xdg-data" / "lofibox" / "groove" / "samples";
    bool found_sample = false;
    if (fs::exists(sample_dir, ec)) {
        for (const auto& entry : fs::directory_iterator(sample_dir)) {
            found_sample = found_sample || entry.path().extension() == ".wav";
        }
    }
    if (!found_sample) {
        std::cerr << "Expected capture overlay OK to save a sample WAV into the XDG groove sample directory.\n";
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
