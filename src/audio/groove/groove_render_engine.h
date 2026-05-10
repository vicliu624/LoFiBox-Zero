// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <optional>

#include "audio/groove/sample_buffer.h"
#include "groove/groove_project.h"

namespace lofibox::audio::groove {

struct GrooveSampleBank {
    std::array<std::optional<SampleBuffer>, lofibox::groove::kGrooveSoundSlotCount> slots{};
};

class GrooveRenderEngine {
public:
    [[nodiscard]] SampleBuffer renderPattern(
        const lofibox::groove::GrooveProject& project,
        std::uint8_t pattern_index,
        const GrooveSampleBank& samples,
        const lofibox::groove::GrooveExportSettings& settings) const;

    [[nodiscard]] SampleBuffer renderSongChain(
        const lofibox::groove::GrooveProject& project,
        const GrooveSampleBank& samples,
        const lofibox::groove::GrooveExportSettings& settings) const;

private:
    [[nodiscard]] SampleBuffer fallbackClick(int sample_rate) const;
};

} // namespace lofibox::audio::groove
