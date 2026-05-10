// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "audio/groove/sample_buffer.h"

namespace lofibox::audio::groove {

struct SampleCaptureRequest {
    std::string sourceTrackId{};
    std::string sourceUri{};

    double startSeconds{0.0};
    double durationSeconds{0.0};

    std::uint8_t targetSoundSlot{0};

    bool normalize{true};
    double fadeInMs{2.0};
    double fadeOutMs{4.0};
};

struct SampleCaptureResult {
    bool ok{false};

    std::string sampleId{};
    std::string sampleUri{};
    std::string displayName{};

    double durationSeconds{0.0};
    std::string errorMessage{};
};

class SampleSegmentDecoder {
public:
    virtual ~SampleSegmentDecoder() = default;
    [[nodiscard]] virtual std::optional<SampleBuffer> decodeSegment(
        std::string_view source_uri,
        double start_seconds,
        double duration_seconds) const = 0;
};

class SampleCaptureService {
public:
    [[nodiscard]] SampleCaptureResult capture(
        const SampleCaptureRequest& request,
        const SampleSegmentDecoder& decoder,
        const std::filesystem::path& sample_dir) const;
};

} // namespace lofibox::audio::groove
