// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/wav_exporter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <fstream>

namespace lofibox::audio::groove {
namespace {

template <typename T>
void writeLe(std::ofstream& out, T value)
{
    for (std::size_t byte = 0; byte < sizeof(T); ++byte) {
        const auto octet = static_cast<char>((static_cast<std::uint64_t>(value) >> (byte * 8U)) & 0xffU);
        out.write(&octet, 1);
    }
}

[[nodiscard]] float normalizeGain(const SampleBuffer& buffer, bool normalize)
{
    if (!normalize) {
        return 1.0f;
    }
    float peak = 0.0f;
    for (float sample : buffer.samples) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak <= 0.000001f ? 1.0f : 0.95f / peak;
}

} // namespace

WavExportResult WavExporter::writePcm16(const std::filesystem::path& path, const SampleBuffer& buffer, bool normalize) const
{
    if (buffer.sampleRate <= 0 || buffer.channels <= 0) {
        return {false, path, "invalid audio format"};
    }
    try {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) {
            return {false, path, "could not open WAV for writing"};
        }

        const std::uint16_t channels = static_cast<std::uint16_t>(buffer.channels);
        const std::uint32_t sample_rate = static_cast<std::uint32_t>(buffer.sampleRate);
        const std::uint16_t bits = 16;
        const std::uint16_t block_align = static_cast<std::uint16_t>(channels * (bits / 8U));
        const std::uint32_t byte_rate = sample_rate * block_align;
        const std::uint32_t data_size = static_cast<std::uint32_t>(buffer.samples.size() * 2U);
        const std::uint32_t riff_size = 36U + data_size;

        out.write("RIFF", 4);
        writeLe<std::uint32_t>(out, riff_size);
        out.write("WAVE", 4);
        out.write("fmt ", 4);
        writeLe<std::uint32_t>(out, 16);
        writeLe<std::uint16_t>(out, 1);
        writeLe<std::uint16_t>(out, channels);
        writeLe<std::uint32_t>(out, sample_rate);
        writeLe<std::uint32_t>(out, byte_rate);
        writeLe<std::uint16_t>(out, block_align);
        writeLe<std::uint16_t>(out, bits);
        out.write("data", 4);
        writeLe<std::uint32_t>(out, data_size);

        const float gain = normalizeGain(buffer, normalize);
        for (float sample : buffer.samples) {
            const float clamped = std::clamp(sample * gain, -1.0f, 1.0f);
            const auto pcm = static_cast<std::int16_t>(std::lrint(clamped * 32767.0f));
            writeLe<std::uint16_t>(out, static_cast<std::uint16_t>(pcm));
        }
        return {static_cast<bool>(out), path, static_cast<bool>(out) ? std::string{} : std::string{"WAV write failed"}};
    } catch (const std::exception& ex) {
        return {false, path, ex.what()};
    }
}

} // namespace lofibox::audio::groove
