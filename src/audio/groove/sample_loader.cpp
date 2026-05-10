// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/sample_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <utility>
#include <vector>

namespace lofibox::audio::groove {
namespace {

template <typename T>
[[nodiscard]] T readLe(const std::vector<unsigned char>& data, std::size_t offset)
{
    T value{};
    for (std::size_t byte = 0; byte < sizeof(T); ++byte) {
        value |= static_cast<T>(data[offset + byte]) << (byte * 8U);
    }
    return value;
}

[[nodiscard]] bool tagEquals(const std::vector<unsigned char>& data, std::size_t offset, const char* tag)
{
    return offset + 4U <= data.size()
        && data[offset] == static_cast<unsigned char>(tag[0])
        && data[offset + 1U] == static_cast<unsigned char>(tag[1])
        && data[offset + 2U] == static_cast<unsigned char>(tag[2])
        && data[offset + 3U] == static_cast<unsigned char>(tag[3]);
}

} // namespace

SampleLoadResult SampleLoader::loadWav(const std::filesystem::path& path) const
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {false, {}, "could not open WAV"};
    }
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>{});
    if (data.size() < 44U || !tagEquals(data, 0, "RIFF") || !tagEquals(data, 8, "WAVE")) {
        return {false, {}, "not a RIFF/WAVE file"};
    }

    std::uint16_t format = 0;
    std::uint16_t channels = 0;
    std::uint32_t sample_rate = 0;
    std::uint16_t bits_per_sample = 0;
    std::size_t pcm_offset = 0;
    std::uint32_t pcm_size = 0;

    std::size_t cursor = 12;
    while (cursor + 8U <= data.size()) {
        const std::uint32_t chunk_size = readLe<std::uint32_t>(data, cursor + 4U);
        const std::size_t payload = cursor + 8U;
        if (payload + chunk_size > data.size()) {
            break;
        }
        if (tagEquals(data, cursor, "fmt ") && chunk_size >= 16U) {
            format = readLe<std::uint16_t>(data, payload);
            channels = readLe<std::uint16_t>(data, payload + 2U);
            sample_rate = readLe<std::uint32_t>(data, payload + 4U);
            bits_per_sample = readLe<std::uint16_t>(data, payload + 14U);
        } else if (tagEquals(data, cursor, "data")) {
            pcm_offset = payload;
            pcm_size = chunk_size;
        }
        cursor = payload + chunk_size + (chunk_size % 2U);
    }

    if (format != 1U || channels == 0U || sample_rate == 0U || bits_per_sample != 16U || pcm_offset == 0U) {
        return {false, {}, "only PCM16 WAV is supported by the built-in sample loader"};
    }

    SampleBuffer buffer{};
    buffer.sampleRate = static_cast<int>(sample_rate);
    buffer.channels = static_cast<int>(channels);
    const auto sample_count = pcm_size / 2U;
    buffer.samples.reserve(sample_count);
    for (std::size_t index = 0; index < sample_count; ++index) {
        const std::uint16_t raw = readLe<std::uint16_t>(data, pcm_offset + (index * 2U));
        const auto signed_value = static_cast<std::int16_t>(raw);
        buffer.samples.push_back(std::clamp(static_cast<float>(signed_value) / 32768.0f, -1.0f, 1.0f));
    }
    return {true, std::move(buffer), {}};
}

} // namespace lofibox::audio::groove
