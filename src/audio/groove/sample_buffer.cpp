// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/sample_buffer.h"

#include <algorithm>
#include <cmath>

namespace lofibox::audio::groove {

std::size_t SampleBuffer::frameCount() const noexcept
{
    if (channels <= 0) {
        return 0;
    }
    return samples.size() / static_cast<std::size_t>(channels);
}

double SampleBuffer::durationSeconds() const noexcept
{
    if (sampleRate <= 0) {
        return 0.0;
    }
    return static_cast<double>(frameCount()) / static_cast<double>(sampleRate);
}

float SampleBuffer::sample(std::size_t frame, int channel) const noexcept
{
    if (channel < 0 || channel >= channels || frame >= frameCount()) {
        return 0.0f;
    }
    return samples[(frame * static_cast<std::size_t>(channels)) + static_cast<std::size_t>(channel)];
}

void SampleBuffer::setSample(std::size_t frame, int channel, float value) noexcept
{
    if (channel < 0 || channel >= channels || frame >= frameCount()) {
        return;
    }
    samples[(frame * static_cast<std::size_t>(channels)) + static_cast<std::size_t>(channel)] = value;
}

void SampleBuffer::appendSilence(std::size_t frames)
{
    samples.resize(samples.size() + (frames * static_cast<std::size_t>(std::max(1, channels))), 0.0f);
}

SampleBuffer makeSilentSampleBuffer(int sample_rate, int channels, double duration_seconds)
{
    SampleBuffer buffer{};
    buffer.sampleRate = std::max(1, sample_rate);
    buffer.channels = std::max(1, channels);
    const auto frames = static_cast<std::size_t>(std::ceil(std::max(0.0, duration_seconds) * static_cast<double>(buffer.sampleRate)));
    buffer.samples.assign(frames * static_cast<std::size_t>(buffer.channels), 0.0f);
    return buffer;
}

} // namespace lofibox::audio::groove
