// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/dsp/dsp_chain.h"
#include "platform/host/runtime_provider_factories.h"

#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {

void writeLe16(std::ofstream& output, std::uint16_t value)
{
    output.put(static_cast<char>(value & 0xFFU));
    output.put(static_cast<char>((value >> 8U) & 0xFFU));
}

void writeLe32(std::ofstream& output, std::uint32_t value)
{
    writeLe16(output, static_cast<std::uint16_t>(value & 0xFFFFU));
    writeLe16(output, static_cast<std::uint16_t>((value >> 16U) & 0xFFFFU));
}

std::filesystem::path writeSineWav()
{
    constexpr int sample_rate = 44100;
    constexpr int channels = 2;
    constexpr int seconds = 4;
    constexpr int bits_per_sample = 16;
    constexpr int frame_count = sample_rate * seconds;
    constexpr int byte_rate = sample_rate * channels * bits_per_sample / 8;
    constexpr int block_align = channels * bits_per_sample / 8;
    constexpr std::uint32_t data_bytes = frame_count * block_align;
    const auto path = std::filesystem::temp_directory_path() / "lofibox-realtime-dsp-smoke.wav";

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write("RIFF", 4);
    writeLe32(output, 36U + data_bytes);
    output.write("WAVEfmt ", 8);
    writeLe32(output, 16U);
    writeLe16(output, 1U);
    writeLe16(output, channels);
    writeLe32(output, sample_rate);
    writeLe32(output, byte_rate);
    writeLe16(output, block_align);
    writeLe16(output, bits_per_sample);
    output.write("data", 4);
    writeLe32(output, data_bytes);

    constexpr double pi = 3.14159265358979323846;
    for (int frame = 0; frame < frame_count; ++frame) {
        const double low = std::sin((2.0 * pi * 125.0 * frame) / sample_rate);
        const double high = std::sin((2.0 * pi * 4000.0 * frame) / sample_rate);
        const auto sample = static_cast<std::int16_t>(std::clamp((low + high) * 0.18, -1.0, 1.0) * 32767.0);
        writeLe16(output, static_cast<std::uint16_t>(sample));
        writeLe16(output, static_cast<std::uint16_t>(sample));
    }
    return path;
}

} // namespace

int main()
{
    if (std::getenv("LOFIBOX_ENABLE_REAL_AUDIO_SMOKE") == nullptr) {
        std::cout << "Set LOFIBOX_ENABLE_REAL_AUDIO_SMOKE=1 to run real ffmpeg/realtime output smoke.\n";
        return 0;
    }

    auto backend = lofibox::platform::host::createHostAudioPlaybackBackend();
    if (!backend || !backend->available()) {
        std::cerr << "Host audio backend is unavailable.\n";
        return 1;
    }

    auto profile = lofibox::audio::dsp::DspChainProfile{};
    profile.eq = lofibox::audio::dsp::tenBandFlatEqProfile();
    profile.eq.enabled = true;
    profile.eq.bands[2].gain_db = 9.0;
    profile.limiter.enabled = true;
    profile.limiter.ceiling_db = -1.0;
    backend->setDspProfile(profile);

    const auto path = writeSineWav();
    if (!backend->playFile(path, 0.0)) {
        std::cerr << "Failed to start realtime DSP playback smoke.\n";
        return 1;
    }

    bool observed_playing = false;
    bool observed_visualization = false;
    bool updated_profile_while_playing = false;
    for (int attempt = 0; attempt < 80; ++attempt) {
        const auto state = backend->state();
        observed_playing = observed_playing || state == lofibox::app::AudioPlaybackState::Playing;
        observed_visualization = observed_visualization || backend->visualizationFrame().available;
        if (observed_playing && !updated_profile_while_playing) {
            profile.eq.bands[2].gain_db = -9.0;
            profile.eq.bands[7].gain_db = 9.0;
            backend->setDspProfile(profile);
            updated_profile_while_playing = true;
        }
        if (observed_playing && observed_visualization && updated_profile_while_playing) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
    backend->stop();

    if (!observed_playing || !observed_visualization || !updated_profile_while_playing) {
        std::cerr << "Realtime DSP playback did not reach playing, visualization, and hot-update observations.\n";
        return 1;
    }
    return 0;
}
