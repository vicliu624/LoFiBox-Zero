// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>

#include "audio/groove/sample_capture_service.h"

namespace lofibox::audio::groove {

class MediaSegmentDecoder final : public SampleSegmentDecoder {
public:
    [[nodiscard]] std::optional<SampleBuffer> decodeSegment(
        std::string_view source_uri,
        double start_seconds,
        double duration_seconds) const override;
    [[nodiscard]] std::string lastErrorMessage() const override;

private:
    mutable std::string lastError_{};
};

} // namespace lofibox::audio::groove
