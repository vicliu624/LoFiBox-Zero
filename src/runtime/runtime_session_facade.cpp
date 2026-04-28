// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_session_facade.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <string>
#include <utility>

#include "audio/dsp/dsp_chain.h"
#include "playback/playback_state.h"

namespace lofibox::runtime {
namespace {

constexpr int kEqMinGainDb = -12;
constexpr int kEqMaxGainDb = 12;

RuntimePlaybackStatus toRuntimeStatus(app::PlaybackStatus status) noexcept
{
    switch (status) {
    case app::PlaybackStatus::Empty: return RuntimePlaybackStatus::Empty;
    case app::PlaybackStatus::Paused: return RuntimePlaybackStatus::Paused;
    case app::PlaybackStatus::Playing: return RuntimePlaybackStatus::Playing;
    }
    return RuntimePlaybackStatus::Empty;
}

std::string upperAscii(std::string_view text)
{
    std::string result{text};
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

audio::dsp::DspChainProfile dspProfileFromEqState(const app::EqState& state)
{
    auto eq = audio::dsp::tenBandFlatEqProfile();
    eq.id = "runtime-current";
    eq.name = state.preset_name.empty() ? std::string{"CUSTOM"} : state.preset_name;

    bool has_gain = false;
    for (std::size_t index = 0; index < eq.bands.size() && index < state.bands.size(); ++index) {
        eq.bands[index].gain_db = static_cast<double>(state.bands[index]);
        has_gain = has_gain || state.bands[index] != 0;
    }
    eq.enabled = state.enabled && has_gain;
    eq.limiter_enabled = true;
    eq.limiter_ceiling_db = -1.0;

    audio::dsp::DspChainProfile profile{};
    profile.active_profile_id = eq.id;
    profile.eq = std::move(eq);
    profile.limiter.enabled = true;
    profile.limiter.ceiling_db = -1.0;
    return profile;
}

bool hasNonZeroGain(const app::EqState& state) noexcept
{
    return std::any_of(state.bands.begin(), state.bands.end(), [](int gain) {
        return gain != 0;
    });
}

} // namespace

RuntimeSessionFacade::RuntimeSessionFacade(application::AppServiceRegistry services, app::EqState& eq) noexcept
    : services_(services),
      eq_(eq)
{
}

void RuntimeSessionFacade::setRemoteTrackStarter(RemoteTrackStarter starter)
{
    remote_track_starter_ = std::move(starter);
}

void RuntimeSessionFacade::setActiveRemoteStreamStarter(ActiveRemoteStreamStarter starter)
{
    active_remote_stream_starter_ = std::move(starter);
}

void RuntimeSessionFacade::setRemoteSessionSnapshotProvider(RemoteSessionSnapshotProvider provider)
{
    remote_session_snapshot_provider_ = std::move(provider);
}

bool RuntimeSessionFacade::playFirstAvailable() const
{
    return services_.playbackCommands().playFirstAvailable(remote_track_starter_);
}

bool RuntimeSessionFacade::startTrack(int track_id) const
{
    if (track_id <= 0) {
        return false;
    }
    return services_.playbackCommands().startTrack(track_id, remote_track_starter_);
}

void RuntimeSessionFacade::pause() const noexcept
{
    services_.playbackCommands().pause();
}

void RuntimeSessionFacade::resume() const noexcept
{
    services_.playbackCommands().resume();
}

void RuntimeSessionFacade::togglePlayPause() const noexcept
{
    services_.playbackCommands().togglePlayPause();
}

bool RuntimeSessionFacade::stepQueue(int delta) const
{
    if (delta == 0) {
        return false;
    }
    services_.queueCommands().step(delta, remote_track_starter_);
    return true;
}

void RuntimeSessionFacade::cycleMainMenuPlaybackMode() const
{
    services_.playbackCommands().cycleMainMenuPlaybackMode();
}

void RuntimeSessionFacade::toggleShuffle() const
{
    services_.playbackCommands().toggleShuffle();
}

void RuntimeSessionFacade::cycleRepeatMode() const noexcept
{
    services_.playbackCommands().cycleRepeatMode();
}

void RuntimeSessionFacade::setRepeatAll(bool enabled) const noexcept
{
    services_.playbackCommands().setRepeatAll(enabled);
}

void RuntimeSessionFacade::setRepeatOne(bool enabled) const noexcept
{
    services_.playbackCommands().setRepeatOne(enabled);
}

bool RuntimeSessionFacade::startActiveRemoteStream() const
{
    return active_remote_stream_starter_ ? active_remote_stream_starter_() : false;
}

void RuntimeSessionFacade::setEqEnabled(bool enabled)
{
    eq_.enabled = enabled;
    applyEqToPlayback();
}

bool RuntimeSessionFacade::setEqBand(int band_index, int gain_db)
{
    if (band_index < 0 || band_index >= static_cast<int>(eq_.bands.size())) {
        return false;
    }
    eq_.bands[static_cast<std::size_t>(band_index)] = std::clamp(gain_db, kEqMinGainDb, kEqMaxGainDb);
    eq_.preset_name = "CUSTOM";
    eq_.enabled = hasNonZeroGain(eq_);
    applyEqToPlayback();
    return true;
}

bool RuntimeSessionFacade::adjustEqBand(int band_index, int delta_db)
{
    if (band_index < 0 || band_index >= static_cast<int>(eq_.bands.size())) {
        return false;
    }
    const auto current = eq_.bands[static_cast<std::size_t>(band_index)];
    return setEqBand(band_index, current + delta_db);
}

bool RuntimeSessionFacade::applyEqPreset(std::string_view preset_name)
{
    const auto wanted = upperAscii(preset_name);
    for (const auto& preset : audio::dsp::builtinEqPresets()) {
        if (upperAscii(preset.name) != wanted) {
            continue;
        }
        for (std::size_t index = 0; index < eq_.bands.size() && index < preset.bands.size(); ++index) {
            eq_.bands[index] = std::clamp(static_cast<int>(std::round(preset.bands[index].gain_db)), kEqMinGainDb, kEqMaxGainDb);
        }
        eq_.preset_name = upperAscii(preset.name);
        eq_.enabled = hasNonZeroGain(eq_);
        applyEqToPlayback();
        return true;
    }
    return false;
}

bool RuntimeSessionFacade::cycleEqPreset(int delta)
{
    auto presets = audio::dsp::builtinEqPresets();
    if (presets.empty()) {
        return false;
    }

    int current = -1;
    const auto current_name = upperAscii(eq_.preset_name);
    for (int index = 0; index < static_cast<int>(presets.size()); ++index) {
        if (upperAscii(presets[static_cast<std::size_t>(index)].name) == current_name) {
            current = index;
            break;
        }
    }

    const int count = static_cast<int>(presets.size());
    const int next = ((current + delta) % count + count) % count;
    return applyEqPreset(presets[static_cast<std::size_t>(next)].name);
}

void RuntimeSessionFacade::resetEq()
{
    eq_.bands.fill(0);
    eq_.preset_name = "FLAT";
    eq_.enabled = false;
    applyEqToPlayback();
}

RuntimeSnapshot RuntimeSessionFacade::snapshot(std::uint64_t version) const
{
    RuntimeSnapshot result{};
    result.version = version;
    result.playback = playbackSnapshot(version);
    result.queue = queueSnapshot(version);
    result.eq = eqSnapshot(version);
    result.remote = remoteSessionSnapshot(version);
    return result;
}

PlaybackRuntimeSnapshot RuntimeSessionFacade::playbackSnapshot(std::uint64_t version) const
{
    const auto status = services_.playbackStatus().snapshot();
    const auto& session = services_.playbackStatus().session();
    PlaybackRuntimeSnapshot result{};
    result.status = toRuntimeStatus(status.status);
    result.current_track_id = status.current_track_id;
    result.title = status.title;
    result.source_label = status.source_label;
    result.elapsed_seconds = status.elapsed_seconds;
    result.duration_seconds = status.duration_seconds;
    result.audio_active = session.audio_active;
    result.shuffle_enabled = status.shuffle_enabled;
    result.repeat_all = status.repeat_all;
    result.repeat_one = status.repeat_one;
    result.version = version;
    return result;
}

QueueRuntimeSnapshot RuntimeSessionFacade::queueSnapshot(std::uint64_t version) const
{
    const auto queue = services_.playbackStatus().queueSnapshot();
    QueueRuntimeSnapshot result{};
    result.active_ids = queue.active_ids;
    result.active_index = queue.active_index;
    result.shuffle_enabled = queue.shuffle_enabled;
    result.repeat_all = queue.repeat_all;
    result.repeat_one = queue.repeat_one;
    result.version = version;
    return result;
}

EqRuntimeSnapshot RuntimeSessionFacade::eqSnapshot(std::uint64_t version) const
{
    EqRuntimeSnapshot result{};
    result.bands = eq_.bands;
    result.preset_name = eq_.preset_name;
    result.enabled = eq_.enabled;
    result.version = version;
    return result;
}

RemoteSessionSnapshot RuntimeSessionFacade::remoteSessionSnapshot(std::uint64_t version) const
{
    auto result = remote_session_snapshot_provider_ ? remote_session_snapshot_provider_() : RemoteSessionSnapshot{};
    result.version = version;
    return result;
}

void RuntimeSessionFacade::applyEqToPlayback() const
{
    services_.playbackCommands().setDspProfile(dspProfileFromEqState(eq_));
}

} // namespace lofibox::runtime
