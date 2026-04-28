// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/dsp/realtime_dsp_engine.h"

#include <cassert>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {

std::vector<float> sine(double frequency, int frames, int sample_rate)
{
    constexpr double pi = 3.14159265358979323846;
    std::vector<float> samples(static_cast<std::size_t>(frames * 2));
    for (int frame = 0; frame < frames; ++frame) {
        const float value = static_cast<float>(std::sin((2.0 * pi * frequency * frame) / sample_rate) * 0.2);
        samples[static_cast<std::size_t>(frame * 2)] = value;
        samples[static_cast<std::size_t>(frame * 2 + 1)] = value;
    }
    return samples;
}

double rms(const std::vector<float>& samples)
{
    double total = 0.0;
    for (const float sample : samples) {
        total += static_cast<double>(sample) * static_cast<double>(sample);
    }
    return std::sqrt(total / static_cast<double>(samples.size()));
}

void processInBlocks(lofibox::audio::dsp::RealtimeDspEngine& engine, std::vector<float>& samples, int sample_rate)
{
    constexpr int channels = 2;
    constexpr int block_frames = 512;
    const int frames = static_cast<int>(samples.size() / channels);
    for (int offset = 0; offset < frames; offset += block_frames) {
        const int count = std::min(block_frames, frames - offset);
        engine.processInterleaved(
            samples.data() + (static_cast<std::size_t>(offset) * channels),
            static_cast<std::size_t>(count),
            channels,
            sample_rate);
    }
}

} // namespace

int main()
{
    constexpr int sample_rate = 48000;
    constexpr int frames = 48000;

    auto profile = lofibox::audio::dsp::DspChainProfile{};
    profile.eq = lofibox::audio::dsp::tenBandFlatEqProfile();
    profile.eq.enabled = true;
    profile.eq.bands[2].gain_db = 12.0;
    profile.limiter.enabled = false;

    lofibox::audio::dsp::RealtimeDspEngine engine{};
    engine.reset(sample_rate, 2);
    engine.setProfile(profile);

    auto low = sine(125.0, frames, sample_rate);
    auto high = sine(4000.0, frames, sample_rate);
    const double low_before = rms(low);
    const double high_before = rms(high);

    processInBlocks(engine, low, sample_rate);
    engine.reset(sample_rate, 2);
    engine.setProfile(profile);
    processInBlocks(engine, high, sample_rate);

    const double low_gain = rms(low) / low_before;
    const double high_gain = rms(high) / high_before;
    assert(low_gain > 1.7);
    assert(high_gain < low_gain * 0.75);
    return 0;
}
