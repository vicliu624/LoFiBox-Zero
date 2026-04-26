// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/dsp/dsp_chain.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string_view>

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

double clampGain(double gain_db)
{
    return std::clamp(gain_db, -24.0, 24.0);
}

EqProfile makeGraphicPreset(std::string id, std::string name, const std::array<double, 10>& gains)
{
    auto profile = tenBandFlatEqProfile();
    profile.id = std::move(id);
    profile.name = std::move(name);
    profile.enabled = true;
    for (std::size_t index = 0; index < profile.bands.size(); ++index) {
        profile.bands[index].gain_db = gains[index];
    }
    return profile;
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
    if (profile_.eq.enabled && !profile_.eq.bypass) {
        gain_db += profile_.eq.preamp_db;
        for (const auto& band : profile_.eq.bands) {
            if (band.enabled) {
                gain_db += band.gain_db / static_cast<double>(profile_.eq.bands.size());
            }
        }
        for (const auto& band : profile_.eq.parametric_bands) {
            if (band.enabled) {
                gain_db += band.gain_db / 12.0;
            }
        }
        if (profile_.eq.loudness_enabled) {
            gain_db += std::clamp(profile_.eq.loudness_strength, 0.0, 1.0) * 3.0;
        }
    }
    if (profile_.replay_gain.enabled) {
        gain_db += profile_.replay_gain.gain_db;
    }
    gain_db += profile_.volume_db;
    double value = static_cast<double>(sample) * dbToLinear(gain_db);
    if (profile_.limiter.enabled || profile_.eq.limiter_enabled) {
        const double ceiling_db = profile_.limiter.enabled ? profile_.limiter.ceiling_db : -1.0;
        const double ceiling = dbToLinear(ceiling_db);
        value = std::clamp(value, -ceiling, ceiling);
    }
    return static_cast<float>(value);
}

EqProfile tenBandFlatEqProfile()
{
    EqProfile profile{};
    profile.id = "flat";
    profile.name = "Flat";
    profile.type = EqProfileType::System;
    profile.enabled = false;
    profile.bypass = false;
    profile.preamp_db = 0.0;
    for (std::size_t index = 0; index < profile.bands.size(); ++index) {
        profile.bands[index].frequency_hz = kGraphicEqFrequencies[index];
        profile.bands[index].gain_db = 0.0;
        profile.bands[index].q = 1.0;
        profile.bands[index].enabled = true;
        profile.bands[index].kind = FilterKind::Bell;
    }
    return profile;
}

EqProfile builtinEqPreset(std::string_view id)
{
    for (auto profile : builtinEqPresets()) {
        if (profile.id == id) {
            return profile;
        }
    }
    return tenBandFlatEqProfile();
}

std::vector<EqProfile> builtinEqPresets()
{
    auto flat = tenBandFlatEqProfile();
    flat.is_default = true;
    return {
        flat,
        makeGraphicPreset("bass_boost", "Bass Boost", {6, 5, 4, 2, 0, 0, 0, -1, -1, -2}),
        makeGraphicPreset("treble_boost", "Treble Boost", {-2, -1, -1, 0, 0, 1, 2, 4, 5, 6}),
        makeGraphicPreset("vocal", "Vocal", {-2, -2, -1, 1, 3, 4, 3, 1, -1, -2}),
        makeGraphicPreset("rock", "Rock", {4, 3, 2, -1, -2, 1, 3, 4, 3, 2}),
        makeGraphicPreset("pop", "Pop", {-1, 2, 3, 2, 0, -1, 1, 2, 3, 2}),
        makeGraphicPreset("jazz", "Jazz", {2, 1, 1, 2, -1, -1, 0, 1, 2, 3}),
        makeGraphicPreset("classical", "Classical", {0, 0, 0, 0, 0, 1, 2, 2, 1, 0}),
        makeGraphicPreset("electronic", "Electronic", {5, 4, 2, 0, -1, 0, 2, 4, 5, 4}),
        makeGraphicPreset("podcast", "Podcast / Speech", {-4, -3, -2, 0, 2, 4, 3, 1, -2, -4}),
    };
}

double smoothedGainDb(double current_db, double target_db, double smoothing)
{
    const double amount = std::clamp(smoothing, 0.0, 1.0);
    return current_db + (target_db - current_db) * amount;
}

PresetRepository::PresetRepository()
{
    for (auto profile : builtinEqPresets()) {
        profiles_.emplace(profile.id, std::move(profile));
    }
}

std::vector<EqProfile> PresetRepository::profiles() const
{
    std::vector<EqProfile> result{};
    result.reserve(profiles_.size());
    for (const auto& [id, profile] : profiles_) {
        result.push_back(profile);
    }
    std::sort(result.begin(), result.end(), [](const EqProfile& left, const EqProfile& right) {
        if (left.type != right.type) {
            return static_cast<int>(left.type) < static_cast<int>(right.type);
        }
        return left.name < right.name;
    });
    return result;
}

std::optional<EqProfile> PresetRepository::profile(std::string_view id) const
{
    const auto found = profiles_.find(std::string(id));
    if (found == profiles_.end()) {
        return std::nullopt;
    }
    return found->second;
}

bool PresetRepository::save(EqProfile profile)
{
    if (profile.id.empty() || profile.name.empty()) {
        return false;
    }
    for (auto& band : profile.bands) {
        band.gain_db = clampGain(band.gain_db);
        band.q = std::clamp(band.q, 0.1, 24.0);
    }
    profile.preamp_db = clampGain(profile.preamp_db);
    profile.balance = std::clamp(profile.balance, -1.0, 1.0);
    profile.loudness_strength = std::clamp(profile.loudness_strength, 0.0, 1.0);
    profiles_[profile.id] = std::move(profile);
    return true;
}

bool PresetRepository::remove(std::string_view id)
{
    const auto found = profiles_.find(std::string(id));
    if (found == profiles_.end() || found->second.type == EqProfileType::System) {
        return false;
    }
    profiles_.erase(found);
    return true;
}

EqProfile PresetRepository::duplicate(std::string_view source_id, std::string id, std::string name)
{
    auto source = profile(source_id).value_or(tenBandFlatEqProfile());
    source.id = std::move(id);
    source.name = std::move(name);
    source.type = EqProfileType::User;
    source.is_default = false;
    (void)save(source);
    return source;
}

bool PresetRepository::rename(std::string_view id, std::string name)
{
    auto found = profiles_.find(std::string(id));
    if (found == profiles_.end() || name.empty() || found->second.type == EqProfileType::System) {
        return false;
    }
    found->second.name = std::move(name);
    return true;
}

std::string PresetRepository::exportProfile(std::string_view id) const
{
    const auto profile_value = profile(id);
    if (!profile_value) {
        return {};
    }
    const auto& profile_ref = *profile_value;
    std::ostringstream output{};
    output << "id=" << profile_ref.id << '\n';
    output << "name=" << profile_ref.name << '\n';
    output << "enabled=" << (profile_ref.enabled ? "1" : "0") << '\n';
    output << "preamp=" << profile_ref.preamp_db << '\n';
    for (std::size_t index = 0; index < profile_ref.bands.size(); ++index) {
        output << "band" << index << '=' << profile_ref.bands[index].gain_db << '\n';
    }
    return output.str();
}

bool PresetRepository::importProfile(std::string_view serialized)
{
    EqProfile profile = tenBandFlatEqProfile();
    profile.type = EqProfileType::User;
    std::istringstream input{std::string(serialized)};
    std::string line{};
    while (std::getline(input, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const auto key = line.substr(0, pos);
        const auto value = line.substr(pos + 1);
        if (key == "id") {
            profile.id = value;
        } else if (key == "name") {
            profile.name = value;
        } else if (key == "enabled") {
            profile.enabled = value == "1";
        } else if (key == "preamp") {
            profile.preamp_db = std::stod(value);
        } else if (key.rfind("band", 0) == 0) {
            const auto index = static_cast<std::size_t>(std::stoul(key.substr(4)));
            if (index < profile.bands.size()) {
                profile.bands[index].gain_db = std::stod(value);
            }
        }
    }
    return save(std::move(profile));
}

EqManager::EqManager()
{
    session_.current_profile_id = "flat";
}

const EqSessionState& EqManager::session() const noexcept
{
    return session_;
}

const PresetRepository& EqManager::presets() const noexcept
{
    return presets_;
}

PresetRepository& EqManager::presets() noexcept
{
    return presets_;
}

std::optional<EqProfile> EqManager::currentProfile() const
{
    return presets_.profile(session_.current_profile_id);
}

DspChainProfile EqManager::chainProfile() const
{
    DspChainProfile result{};
    result.active_profile_id = session_.current_profile_id;
    if (auto profile = currentProfile()) {
        profile->enabled = session_.enabled;
        profile->bypass = session_.bypass;
        result.eq = *profile;
        result.replay_gain.enabled = profile->replaygain_mode != ReplayGainMode::Off;
        result.replay_gain.mode = profile->replaygain_mode;
        result.limiter.enabled = profile->limiter_enabled;
        result.limiter.ceiling_db = profile->limiter_ceiling_db;
    }
    return result;
}

bool EqManager::selectProfile(std::string_view id)
{
    if (!presets_.profile(id)) {
        return false;
    }
    session_.current_profile_id = std::string(id);
    return true;
}

void EqManager::setEnabled(bool enabled) noexcept
{
    session_.enabled = enabled;
}

void EqManager::setBypass(bool bypass) noexcept
{
    session_.bypass = bypass;
}

bool EqManager::saveCurrent(EqProfile profile)
{
    profile.id = session_.current_profile_id;
    return presets_.save(std::move(profile));
}

bool EqManager::updateGraphicBand(std::size_t band_index, double gain_db)
{
    auto profile = currentProfile();
    if (!profile || band_index >= profile->bands.size()) {
        return false;
    }
    profile->bands[band_index].gain_db = gain_db;
    return saveCurrent(std::move(*profile));
}

bool EqManager::updatePreamp(double preamp_db)
{
    auto profile = currentProfile();
    if (!profile) {
        return false;
    }
    profile->preamp_db = preamp_db;
    return saveCurrent(std::move(*profile));
}

bool EqManager::setBalance(double balance)
{
    auto profile = currentProfile();
    if (!profile) {
        return false;
    }
    profile->balance = balance;
    return saveCurrent(std::move(*profile));
}

bool EqManager::setLoudness(bool enabled, double strength)
{
    auto profile = currentProfile();
    if (!profile) {
        return false;
    }
    profile->loudness_enabled = enabled;
    profile->loudness_strength = strength;
    return saveCurrent(std::move(*profile));
}

bool EqManager::setLimiter(bool enabled, double ceiling_db)
{
    auto profile = currentProfile();
    if (!profile) {
        return false;
    }
    profile->limiter_enabled = enabled;
    profile->limiter_ceiling_db = ceiling_db;
    return saveCurrent(std::move(*profile));
}

bool EqManager::setReplayGain(ReplayGainMode mode, double, double)
{
    auto profile = currentProfile();
    if (!profile) {
        return false;
    }
    profile->replaygain_mode = mode;
    return saveCurrent(std::move(*profile));
}

bool EqManager::setHighPass(std::optional<double> cutoff_hz)
{
    auto profile = currentProfile();
    if (!profile) {
        return false;
    }
    profile->high_pass_hz = cutoff_hz;
    return saveCurrent(std::move(*profile));
}

bool EqManager::setLowPass(std::optional<double> cutoff_hz)
{
    auto profile = currentProfile();
    if (!profile) {
        return false;
    }
    profile->low_pass_hz = cutoff_hz;
    return saveCurrent(std::move(*profile));
}

bool EqManager::setParametricBands(std::vector<ParametricEqBand> bands)
{
    auto profile = currentProfile();
    if (!profile) {
        return false;
    }
    profile->parametric_bands = std::move(bands);
    return saveCurrent(std::move(*profile));
}

bool EqManager::bindOutputDevice(std::string output_device_id, std::string profile_id)
{
    if (!presets_.profile(profile_id) || output_device_id.empty()) {
        return false;
    }
    auto found = std::find_if(device_bindings_.begin(), device_bindings_.end(), [&](const OutputDeviceBinding& binding) {
        return binding.output_device_id == output_device_id;
    });
    if (found == device_bindings_.end()) {
        device_bindings_.push_back({std::move(output_device_id), std::move(profile_id)});
    } else {
        found->eq_profile_id = std::move(profile_id);
    }
    return true;
}

bool EqManager::bindContentType(std::string content_type, std::string profile_id)
{
    if (!presets_.profile(profile_id) || content_type.empty()) {
        return false;
    }
    auto found = std::find_if(content_bindings_.begin(), content_bindings_.end(), [&](const ContentTypeBinding& binding) {
        return binding.content_type == content_type;
    });
    if (found == content_bindings_.end()) {
        content_bindings_.push_back({std::move(content_type), std::move(profile_id)});
    } else {
        found->eq_profile_id = std::move(profile_id);
    }
    return true;
}

std::optional<std::string> EqManager::profileForOutputDevice(std::string_view output_device_id) const
{
    const auto found = std::find_if(device_bindings_.begin(), device_bindings_.end(), [&](const OutputDeviceBinding& binding) {
        return binding.output_device_id == output_device_id;
    });
    if (found == device_bindings_.end()) {
        return std::nullopt;
    }
    return found->eq_profile_id;
}

std::optional<std::string> EqManager::profileForContentType(std::string_view content_type) const
{
    const auto found = std::find_if(content_bindings_.begin(), content_bindings_.end(), [&](const ContentTypeBinding& binding) {
        return binding.content_type == content_type;
    });
    if (found == content_bindings_.end()) {
        return std::nullopt;
    }
    return found->eq_profile_id;
}

} // namespace lofibox::audio::dsp
