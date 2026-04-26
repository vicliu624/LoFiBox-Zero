// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace lofibox::audio::decoder {

struct AudioStreamInfo {
    std::string codec{};
    int sample_rate_hz{0};
    int channels{0};
    int bitrate_bps{0};
    double duration_seconds{0.0};
};

struct DecodedAudioChunk {
    std::vector<float> interleaved_samples{};
    int sample_rate_hz{0};
    int channels{0};
    std::uint64_t frame_index{0};
};

class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;

    [[nodiscard]] virtual bool open(const std::filesystem::path& path) = 0;
    [[nodiscard]] virtual std::optional<AudioStreamInfo> streamInfo() const = 0;
    [[nodiscard]] virtual std::optional<DecodedAudioChunk> readChunk() = 0;
    virtual void close() noexcept = 0;
};

} // namespace lofibox::audio::decoder
