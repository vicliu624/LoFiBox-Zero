// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/punch_fx_processor.h"

#include <algorithm>
#include <cmath>

namespace lofibox::audio::groove {

void PunchFxProcessor::process(SampleBuffer& buffer, PunchFxType type, float amount) const
{
    const float mix = std::clamp(amount, 0.0f, 1.0f);
    if (type == PunchFxType::None || buffer.samples.empty()) {
        return;
    }

    if (type == PunchFxType::Reverse) {
        const auto frames = buffer.frameCount();
        for (std::size_t left = 0; left < frames / 2U; ++left) {
            const auto right = frames - 1U - left;
            for (int channel = 0; channel < buffer.channels; ++channel) {
                const float a = buffer.sample(left, channel);
                const float b = buffer.sample(right, channel);
                buffer.setSample(left, channel, b);
                buffer.setSample(right, channel, a);
            }
        }
        return;
    }

    if (type == PunchFxType::Bitcrush) {
        const float steps = 8.0f + ((1.0f - mix) * 56.0f);
        for (float& sample : buffer.samples) {
            sample = std::round(sample * steps) / steps;
        }
        return;
    }

    if (type == PunchFxType::Filter) {
        for (int channel = 0; channel < buffer.channels; ++channel) {
            float last = 0.0f;
            for (std::size_t frame = 0; frame < buffer.frameCount(); ++frame) {
                const float current = buffer.sample(frame, channel);
                last = (last * (0.82f + mix * 0.14f)) + (current * (0.18f - mix * 0.14f));
                buffer.setSample(frame, channel, last);
            }
        }
        return;
    }

    if (type == PunchFxType::Stutter) {
        const auto loop_frames = static_cast<std::size_t>(std::max(1, buffer.sampleRate / 32));
        for (std::size_t frame = loop_frames; frame < buffer.frameCount(); ++frame) {
            for (int channel = 0; channel < buffer.channels; ++channel) {
                const float repeated = buffer.sample(frame % loop_frames, channel);
                const float dry = buffer.sample(frame, channel);
                buffer.setSample(frame, channel, (dry * (1.0f - mix)) + (repeated * mix));
            }
        }
        return;
    }

    if (type == PunchFxType::TapeStop || type == PunchFxType::VinylBrake) {
        const auto frames = buffer.frameCount();
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const float fade = 1.0f - (static_cast<float>(frame) / static_cast<float>(std::max<std::size_t>(1, frames - 1U))) * mix;
            for (int channel = 0; channel < buffer.channels; ++channel) {
                buffer.setSample(frame, channel, buffer.sample(frame, channel) * fade);
            }
        }
        return;
    }

    if (type == PunchFxType::DelayThrow || type == PunchFxType::ReverbFreeze) {
        const auto delay = static_cast<std::size_t>(std::max(1, buffer.sampleRate / (type == PunchFxType::DelayThrow ? 4 : 12)));
        for (std::size_t frame = delay; frame < buffer.frameCount(); ++frame) {
            for (int channel = 0; channel < buffer.channels; ++channel) {
                const float dry = buffer.sample(frame, channel);
                const float wet = buffer.sample(frame - delay, channel) * (type == PunchFxType::DelayThrow ? 0.45f : 0.70f);
                buffer.setSample(frame, channel, std::clamp(dry + wet * mix, -1.0f, 1.0f));
            }
        }
    }
}

} // namespace lofibox::audio::groove
