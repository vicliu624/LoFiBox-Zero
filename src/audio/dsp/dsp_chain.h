// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <vector>

namespace lofibox::audio::dsp {

struct EqBand {
    double frequency_hz{1000.0};
    double gain_db{0.0};
    double q{1.0};
    bool enabled{true};
};

struct EqProfile {
    bool enabled{false};
    double preamp_db{0.0};
    std::array<EqBand, 10> bands{};
};

struct ReplayGainProfile {
    bool enabled{false};
    double gain_db{0.0};
};

struct LimiterProfile {
    bool enabled{true};
    double ceiling_db{-1.0};
};

struct DspChainProfile {
    EqProfile eq{};
    ReplayGainProfile replay_gain{};
    LimiterProfile limiter{};
};

class DspChain {
public:
    void setProfile(DspChainProfile profile);
    [[nodiscard]] const DspChainProfile& profile() const noexcept;
    [[nodiscard]] float processSample(float sample) const noexcept;

private:
    DspChainProfile profile_{};
};

[[nodiscard]] EqProfile tenBandFlatEqProfile();

} // namespace lofibox::audio::dsp
