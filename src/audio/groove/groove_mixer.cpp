// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/groove_mixer.h"

#include <algorithm>

namespace lofibox::audio::groove {

void GrooveMixer::mixInto(SampleBuffer& target, const SampleBuffer& source, std::size_t start_frame, float gain, float pan) const noexcept
{
    if (target.channels <= 0 || source.channels <= 0) {
        return;
    }
    const float left_gain = gain * (pan <= 0.0f ? 1.0f : 1.0f - std::min(1.0f, pan));
    const float right_gain = gain * (pan >= 0.0f ? 1.0f : 1.0f + std::max(-1.0f, pan));
    for (std::size_t frame = 0; frame < source.frameCount(); ++frame) {
        const auto target_frame = start_frame + frame;
        if (target_frame >= target.frameCount()) {
            return;
        }
        for (int channel = 0; channel < target.channels; ++channel) {
            const int source_channel = std::min(channel, source.channels - 1);
            const float channel_gain = channel == 0 ? left_gain : right_gain;
            const float mixed = target.sample(target_frame, channel) + (source.sample(frame, source_channel) * channel_gain);
            target.setSample(target_frame, channel, mixed);
        }
    }
}

void GrooveMixer::clamp(SampleBuffer& target) const noexcept
{
    for (float& sample : target.samples) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }
}

} // namespace lofibox::audio::groove
