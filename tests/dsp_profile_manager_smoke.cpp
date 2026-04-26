// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/dsp/dsp_chain.h"

#include <cassert>
#include <cmath>

int main()
{
    lofibox::audio::dsp::PresetRepository repository{};
    const auto builtins = repository.profiles();
    assert(builtins.size() >= 10);
    assert(repository.profile("flat"));
    assert(repository.profile("bass_boost"));

    auto custom = repository.duplicate("flat", "my-headphones", "My Headphones");
    custom.enabled = true;
    custom.preamp_db = -4.0;
    custom.bands[0].gain_db = 5.0;
    custom.balance = 0.25;
    custom.loudness_enabled = true;
    custom.loudness_strength = 0.5;
    custom.limiter_enabled = true;
    custom.limiter_ceiling_db = -2.0;
    custom.replaygain_mode = lofibox::audio::dsp::ReplayGainMode::Track;
    custom.parametric_bands.push_back({3200.0, -2.5, 4.0, true});
    custom.high_pass_hz = 40.0;
    custom.low_pass_hz = 18000.0;
    assert(repository.save(custom));
    assert(repository.rename("my-headphones", "My Headphones V2"));
    const auto exported = repository.exportProfile("my-headphones");
    assert(exported.find("band0=5") != std::string::npos);
    assert(repository.importProfile(exported));
    assert(repository.remove("my-headphones"));
    assert(!repository.remove("flat"));

    lofibox::audio::dsp::EqManager manager{};
    manager.presets().save(custom);
    assert(manager.selectProfile("my-headphones"));
    manager.setEnabled(true);
    manager.setBypass(false);
    assert(manager.updateGraphicBand(1, 3.0));
    assert(manager.updatePreamp(-6.0));
    assert(manager.setBalance(-0.2));
    assert(manager.setLoudness(true, 0.75));
    assert(manager.setLimiter(true, -3.0));
    assert(manager.setReplayGain(lofibox::audio::dsp::ReplayGainMode::Album, -5.0, -18.0));
    assert(manager.setHighPass(55.0));
    assert(manager.setLowPass(16000.0));
    assert(manager.setParametricBands({{1000.0, 2.0, 1.2, true}}));
    assert(manager.bindOutputDevice("usb-dac", "my-headphones"));
    assert(manager.bindContentType("speech", "podcast"));
    assert(manager.profileForOutputDevice("usb-dac").value() == "my-headphones");
    assert(manager.profileForContentType("speech").value() == "podcast");

    const auto chain_profile = manager.chainProfile();
    assert(chain_profile.eq.enabled);
    assert(chain_profile.eq.high_pass_hz.value() == 55.0);
    assert(chain_profile.eq.low_pass_hz.value() == 16000.0);
    assert(chain_profile.limiter.enabled);
    assert(chain_profile.limiter.ceiling_db == -3.0);

    lofibox::audio::dsp::DspChain chain{};
    chain.setProfile(chain_profile);
    const float processed = chain.processSample(1.0F);
    assert(processed < 1.0F);
    assert(processed > 0.6F);

    const double smoothed = lofibox::audio::dsp::smoothedGainDb(0.0, 10.0, 0.25);
    assert(std::fabs(smoothed - 2.5) < 0.001);
    return 0;
}
