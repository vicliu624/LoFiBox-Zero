// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/audio_pipeline_controller.h"
#include "audio/dsp/dsp_chain.h"

#include <cassert>
#include <cmath>

int main()
{
    const auto eq = lofibox::audio::dsp::tenBandFlatEqProfile();
    assert(eq.bands[0].frequency_hz == 31.0);
    assert(eq.bands[9].frequency_hz == 16000.0);

    lofibox::audio::dsp::DspChainProfile profile{};
    profile.eq = eq;
    profile.eq.enabled = true;
    profile.eq.preamp_db = 6.0;
    profile.replay_gain.enabled = true;
    profile.replay_gain.gain_db = -6.0;
    profile.limiter.enabled = true;
    profile.limiter.ceiling_db = -1.0;

    lofibox::audio::AudioPipelineController pipeline{};
    pipeline.setDspProfile(profile);
    assert(pipeline.dspProfile().eq.enabled);

    const float unchanged = pipeline.processDspSample(0.5f);
    assert(std::fabs(unchanged - 0.5f) < 0.01f);

    profile.replay_gain.gain_db = 0.0;
    pipeline.setDspProfile(profile);
    const float limited = pipeline.processDspSample(1.0f);
    assert(limited < 1.0f);
    assert(limited > 0.85f);
    return 0;
}
