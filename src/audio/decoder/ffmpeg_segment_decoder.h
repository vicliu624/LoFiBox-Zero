// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "audio/decoder/audio_decoder_contract.h"

namespace lofibox::audio::decoder {

class FfmpegSegmentDecoder final {
public:
    [[nodiscard]] std::optional<DecodedAudioChunk> decodeSegment(
        std::string_view source_uri,
        double start_seconds,
        double duration_seconds,
        int output_sample_rate_hz = 48000,
        int output_channels = 2) const;

    [[nodiscard]] std::string lastErrorMessage() const;

private:
    mutable std::string lastError_{};
};

} // namespace lofibox::audio::decoder
