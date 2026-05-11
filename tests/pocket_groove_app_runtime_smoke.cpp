// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

#include "app/input_event.h"
#include "app/lofibox_app.h"
#include "app/runtime_services.h"
#include "audio/groove/sample_buffer.h"
#include "audio/groove/wav_exporter.h"
#include "core/canvas.h"
#include "groove/groove_project.h"
#include "media_fixture_utils.h"
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

std::optional<fs::path> resolveFfmpeg()
{
#if defined(_WIN32)
    return test_media_fixture::resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe");
#elif defined(__linux__)
    return test_media_fixture::resolveExecutablePath("FFMPEG_PATH", "ffmpeg");
#else
    return std::nullopt;
#endif
}

bool transcodeToMp3(const fs::path& ffmpeg, const fs::path& source_wav, const fs::path& target_mp3)
{
    const std::vector<std::string> args{
        "-hide_banner",
        "-loglevel",
        "error",
        "-nostdin",
        "-i",
        source_wav.string(),
        "-c:a",
        "libmp3lame",
        "-y",
        target_mp3.string(),
    };
    return test_media_fixture::runCommand(ffmpeg, args) && fs::exists(target_mp3);
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

std::optional<fs::path> findFirstFileWithExtension(const fs::path& dir, const std::string& extension)
{
    std::error_code ec{};
    if (!fs::exists(dir, ec)) {
        return std::nullopt;
    }
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!ec && entry.is_regular_file() && entry.path().extension() == extension) {
            return entry.path();
        }
    }
    return std::nullopt;
}

std::string readText(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool hasPlayableWavHeader(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::string header(44, '\0');
    file.read(header.data(), static_cast<std::streamsize>(header.size()));
    if (file.gcount() != 44) {
        return false;
    }
    const auto data_size =
        static_cast<unsigned int>(static_cast<unsigned char>(header[40])) |
        (static_cast<unsigned int>(static_cast<unsigned char>(header[41])) << 8U) |
        (static_cast<unsigned int>(static_cast<unsigned char>(header[42])) << 16U) |
        (static_cast<unsigned int>(static_cast<unsigned char>(header[43])) << 24U);
    return header.substr(0, 4) == "RIFF" && header.substr(8, 4) == "WAVE" &&
        header.substr(12, 4) == "fmt " && header.substr(36, 4) == "data" &&
        data_size > 0U && fs::file_size(path) > 44U;
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
    if (const auto ffmpeg = resolveFfmpeg()) {
        const auto source_wav = root / "capture-source.wav";
        writeTestWav(source_wav);
        if (!transcodeToMp3(*ffmpeg, source_wav, root / "Artist" / "Album" / "alpha.mp3")) {
            std::cerr << "Expected ffmpeg to generate MP3 fixture for current-track Groove capture.\n";
            return 1;
        }
        fs::remove(source_wav, ec);
    } else {
        std::cout << "ffmpeg not found; Pocket Groove runtime capture falls back to WAV fixture.\n";
        writeTestWav(root / "Artist" / "Album" / "alpha.wav");
    }

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
        std::cerr << "Expected one local track to be indexed for groove capture.\n";
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

    press(app, lofibox::app::InputKey::Enter, "OK");
    press(app, lofibox::app::InputKey::F5, "F5");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Down, "DOWN");
    press(app, lofibox::app::InputKey::Down, "DOWN");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Backspace, "BACK");

    press(app, lofibox::app::InputKey::F6, "F6");
    press(app, lofibox::app::InputKey::Enter, "OK");
    press(app, lofibox::app::InputKey::Backspace, "BACK");

    press(app, lofibox::app::InputKey::F9, "F9");
    press(app, lofibox::app::InputKey::Enter, "OK");
    const auto export_dir = root / "home" / "Music" / "LoFiBox" / "Exports";
    const auto exported = findFirstFileWithExtension(export_dir, ".wav");
    if (!exported || !hasPlayableWavHeader(*exported)) {
        std::cerr << "Expected Export overlay OK to create a playable WAV in the XDG export target.\n";
        return 1;
    }
    press(app, lofibox::app::InputKey::Backspace, "BACK");
    if (app.snapshot().current_page != lofibox::app::AppPage::PocketGroove) {
        std::cerr << "Expected Back from Export overlay to stay on Pocket Groove.\n";
        return 1;
    }

    press(app, lofibox::app::InputKey::F10, "F10");
    if (app.snapshot().current_page != lofibox::app::AppPage::PocketGroove) {
        std::cerr << "Expected Project overlay shortcut to stay on Pocket Groove.\n";
        return 1;
    }
    press(app, lofibox::app::InputKey::Enter, "OK");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    press(app, lofibox::app::InputKey::Enter, "OK");

    const auto project_dir = root / "xdg-data" / "lofibox" / "groove" / "projects";
    const auto project_file = findFirstFileWithExtension(project_dir, ".json");
    if (!project_file) {
        std::cerr << "Expected Project overlay Save to write a Groove project JSON file.\n";
        return 1;
    }
    const auto project = lofibox::groove::grooveProjectFromJson(readText(*project_file));
    if (project.sounds[0].sourceUri.empty() || project.sounds[0].startSeconds <= 0.0 || project.sounds[0].gain <= 1.0f) {
        std::cerr << "Expected saved project to preserve captured slot and sample edit parameters.\n";
        return 1;
    }
    if (!project.patterns[project.activePattern].tracks[0].steps[0].trigger || project.songChain.items.empty()) {
        std::cerr << "Expected saved project to preserve step edit and song chain item.\n";
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
