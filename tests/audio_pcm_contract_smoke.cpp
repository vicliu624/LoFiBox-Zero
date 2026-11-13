// SPDX-License-Identifier: GPL-3.0-or-later
// Real decoder -> product DSP/clock -> capturing PCM sink, with no audio device.
#include "audio/pcm_format.h"
#include "platform/host/runtime_host_internal.h"
#include "platform/host/runtime_provider_factories.h"
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;
using namespace std::chrono_literals;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
int sink(int argc, char** argv)
{
    int rate{}, channels{};
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string_view{argv[i]} == "--rate") rate = std::stoi(argv[i + 1]);
        if (std::string_view{argv[i]} == "--channels") channels = std::stoi(argv[i + 1]);
    }
    std::array<char, 8192 + 8> bytes{};
    std::size_t carry{}, total{}, crossings{};
    float previous{};
    while (true) {
        ssize_t count;
        do { count = ::read(STDIN_FILENO, bytes.data() + carry, 8192); } while (count < 0 && errno == EINTR);
        if (count < 0) return 2;
        if (!count) break;
        total += static_cast<std::size_t>(count);
        const auto available = carry + static_cast<std::size_t>(count);
        const auto complete = available / 8 * 8;
        for (std::size_t i = 0; i < complete; i += 8) {
            float left;
            std::memcpy(&left, bytes.data() + i, sizeof(left));
            if (!std::isfinite(left)) return 3;
            if (previous <= 0 && left > 0) ++crossings;
            previous = left;
        }
        carry = available - complete;
        std::memmove(bytes.data(), bytes.data() + complete, carry);
    }
    if (carry) return 4;
    std::ofstream{std::getenv("LOFIBOX_PCM_TEST_REPORT")} << rate << ' ' << channels << ' ' << total << ' ' << crossings;
    return 0;
}
struct Cleanup {
    fs::path root;
    ~Cleanup() { std::error_code ignored; fs::remove_all(root, ignored); }
};
} // namespace

int main(int argc, char** argv)
{
    if (fs::path{argv[0]}.filename() == "pw-cat") return sink(argc, argv);
    if (argc != 3) return 2;
    try {
        const auto root = fs::temp_directory_path() / ("lofibox-pcm-contract-" + std::to_string(::getpid()));
        Cleanup cleanup{root}; fs::create_directories(root);
        fs::create_symlink(fs::canonical("/proc/self/exe"), root / "pw-cat");
        std::ofstream{root / "pipewire-0"};
        const auto mp3 = root / "input.mp3";
        const auto generated = lofibox::platform::host::runtime_detail::captureProcessOutput(argv[2], {
            "-hide_banner", "-loglevel", "error", "-nostdin", "-f", "lavfi", "-i",
            "sine=frequency=470:sample_rate=44100:duration=2.75", "-ac", "2", "-c:a", "libmp3lame", "-q:a", "2", mp3.string()});
        require(generated.has_value() && fs::file_size(mp3) > 0, "failed to generate MP3 fixture");
        setenv("PATH", root.c_str(), 1);
        setenv("FFMPEG_PATH", argv[2], 1);
        setenv("FFPLAY_PATH", "/bin/true", 1);
        setenv("PIPEWIRE_RUNTIME_DIR", root.c_str(), 1);
        const auto report = root / "sink-report";
        setenv("LOFIBOX_PCM_TEST_REPORT", report.c_str(), 1);
        for (const bool native : {true, false}) {
            fs::remove(report);
            setenv("LOFIBOX_MP3_DECODER_PATH", native ? argv[1] : "", 1);
            auto preferences = std::make_shared<lofibox::app::RuntimePreferences>();
            auto backend = lofibox::platform::host::createHostAudioPlaybackBackend(preferences);
            require(backend && backend->available(), "product backend unavailable");
            const auto started = std::chrono::steady_clock::now();
            require(backend->playFile(mp3, 0), "failed to start product backend");
            double position{};
            bool suppressed = false, restored = false, saw_blank = false, saw_restored = false;
            while (!fs::exists(report)) {
                if (const auto value = backend->positionSeconds()) position = *value;
                if (!suppressed && position > 0.3) {
                    preferences->toggle(lofibox::app::RuntimeFeature::BatterySaver);
                    suppressed = true;
                }
                if (suppressed && !restored && !backend->visualizationFrame().available) saw_blank = true;
                if (!restored && position > 0.9) {
                    require(saw_blank, "disabled visualizations continued publishing frames");
                    preferences->toggle(lofibox::app::RuntimeFeature::BatterySaver);
                    restored = true;
                }
                if (restored && backend->visualizationFrame().available) saw_restored = true;
                require(std::chrono::steady_clock::now() - started < 9s, "PCM sink did not reach EOF");
                std::this_thread::sleep_for(5ms);
            }
            if (const auto value = backend->positionSeconds()) position = *value;
            backend->stop(); backend.reset();
            require(saw_blank && saw_restored, "live visualization disable/restore failed");
            int rate{}, channels{}; std::size_t bytes{}, crossings{};
            std::ifstream{report} >> rate >> channels >> bytes >> crossings;
            require(rate == 48000 && channels == 2, "sink PCM format differs from product contract");
            require(bytes == 48000 * 8 * 2.75, "PCM duration differs from source at the actual sink rate");
            require(std::abs(static_cast<double>(crossings) / 2.75 - 470) < 2, "source pitch changed in product pipeline");
            require(position > 2.62 && position < 2.76, "product playback clock disagrees with source duration");
            // FFmpeg's existing -re path has an initial read-ahead burst. The
            // strict no-catch-up pacing assertion belongs to our own producer;
            // sample count, pitch and product clock are checked for both.
            const auto wall_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
            require(!native || wall_ms > 2500, "native producer accelerated playback");
            std::cout << (native ? "native" : "ffmpeg") << ": rate=" << rate << " frames=" << bytes / 8 << " position=" << position << " wall_ms=" << wall_ms << '\n';
        }
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
