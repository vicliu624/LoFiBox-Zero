// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/sample_voice.h"

#include <algorithm>

namespace lofibox::audio::groove {

void SampleVoice::trigger(const SampleBuffer& buffer, float voice_gain) noexcept
{
    source = &buffer;
    positionFrame = 0;
    gain = voice_gain;
    active = true;
}

float SampleVoice::nextSample(int channel) noexcept
{
    if (!active || source == nullptr || positionFrame >= source->frameCount()) {
        active = false;
        return 0.0f;
    }
    const float value = source->sample(positionFrame, std::clamp(channel, 0, source->channels - 1)) * gain;
    if (channel >= source->channels - 1) {
        ++positionFrame;
    }
    if (positionFrame >= source->frameCount()) {
        active = false;
    }
    return value;
}

} // namespace lofibox::audio::groove
