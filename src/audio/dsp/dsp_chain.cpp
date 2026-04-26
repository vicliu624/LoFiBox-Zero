// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/dsp/dsp_chain.h"

#include <algorithm>
#include <cmath>

namespace lofibox::audio::dsp {
namespace {

constexpr std::array<double, 10> kGraphicEqFrequencies{
    31.0,
    62.0,
    125.0,
    250.0,
    500.0,
    1000.0,
    2000.0,
    4000.0,
    8000.0,
    16000.0,
};

double dbToLinear(double db)
{
    return std::pow(10.0, db / 20.0);
}

} // namespace

void DspChain::setProfile(DspChainProfile profile)
{
    profile_ = profile;
}

const DspChainProfile& DspChain::profile() const noexcept
{
    return profile_;
}

float DspChain::processSample(float sample) const noexcept
{
    double gain_db = 0.0;
    if (profile_.eq.enabled) {
        gain_db += profile_.eq.preamp_db;
    }
    if (profile_.replay_gain.enabled) {
        gain_db += profile_.replay_gain.gain_db;
    }
    double value = static_cast<double>(sample) * dbToLinear(gain_db);
    if (profile_.limiter.enabled) {
        const double ceiling = dbToLinear(profile_.limiter.ceiling_db);
        value = std::clamp(value, -ceiling, ceiling);
    }
    return static_cast<float>(value);
}

EqProfile tenBandFlatEqProfile()
{
    EqProfile profile{};
    profile.enabled = false;
    profile.preamp_db = 0.0;
    for (std::size_t index = 0; index < profile.bands.size(); ++index) {
        profile.bands[index].frequency_hz = kGraphicEqFrequencies[index];
        profile.bands[index].gain_db = 0.0;
        profile.bands[index].q = 1.0;
        profile.bands[index].enabled = true;
    }
    return profile;
}

} // namespace lofibox::audio::dsp
