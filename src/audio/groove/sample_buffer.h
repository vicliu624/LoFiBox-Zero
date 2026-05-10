// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <vector>

namespace lofibox::audio::groove {

struct SampleBuffer {
    int sampleRate{48000};
    int channels{1};
    std::vector<float> samples{};

    [[nodiscard]] std::size_t frameCount() const noexcept;
    [[nodiscard]] double durationSeconds() const noexcept;
    [[nodiscard]] float sample(std::size_t frame, int channel) const noexcept;
    void setSample(std::size_t frame, int channel, float value) noexcept;
    void appendSilence(std::size_t frames);
};

[[nodiscard]] SampleBuffer makeSilentSampleBuffer(int sample_rate, int channels, double duration_seconds);

} // namespace lofibox::audio::groove
