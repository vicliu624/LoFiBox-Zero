// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_command_dispatcher.h"

namespace lofibox::runtime {

RuntimeCommandDispatcher::RuntimeCommandDispatcher(RuntimeSessionFacade& session) noexcept
    : session_(session)
{
}

RuntimeCommandResult RuntimeCommandDispatcher::dispatch(const RuntimeCommand& command)
{
    switch (command.kind) {
    case RuntimeCommandKind::PlaybackPlay:
        return applied(command, "PLAYBACK_PLAY", "Playback play submitted.", session_.playback().playFirstAvailable());
    case RuntimeCommandKind::PlaybackPause:
        session_.playback().pause();
        return applied(command, "PLAYBACK_PAUSE", "Playback pause submitted.", true);
    case RuntimeCommandKind::PlaybackResume:
        session_.playback().resume();
        return applied(command, "PLAYBACK_RESUME", "Playback resume submitted.", true);
    case RuntimeCommandKind::PlaybackToggle:
        session_.playback().togglePlayPause();
        return applied(command, "PLAYBACK_TOGGLE", "Playback toggle submitted.", true);
    case RuntimeCommandKind::PlaybackStartTrack:
        {
            const auto* payload = command.payload.get<PlaybackStartTrackPayload>();
            return applied(command, "PLAYBACK_START_TRACK", "Playback start-track submitted.", payload != nullptr && session_.playback().startTrack(payload->track_id));
        }
    case RuntimeCommandKind::QueueStep:
        {
            const auto* payload = command.payload.get<QueueStepPayload>();
            return applied(command, "QUEUE_STEP", "Queue step submitted.", payload != nullptr && session_.queue().step(payload->delta));
        }
    case RuntimeCommandKind::PlaybackToggleShuffle:
        session_.queue().toggleShuffle();
        return applied(command, "PLAYBACK_SHUFFLE", "Playback shuffle submitted.", true);
    case RuntimeCommandKind::PlaybackCycleRepeat:
        session_.queue().cycleRepeatMode();
        return applied(command, "PLAYBACK_REPEAT", "Playback repeat mode submitted.", true);
    case RuntimeCommandKind::PlaybackCycleMainMenuMode:
        session_.queue().cycleMainMenuPlaybackMode();
        return applied(command, "PLAYBACK_MENU_MODE", "Main-menu playback mode submitted.", true);
    case RuntimeCommandKind::PlaybackSetRepeatAll:
        {
            const auto* payload = command.payload.get<RuntimeEnabledPayload>();
            if (payload == nullptr) {
                return applied(command, "PLAYBACK_REPEAT_ALL", "Repeat-all payload missing.", false);
            }
            session_.queue().setRepeatAll(payload->enabled);
            return applied(command, "PLAYBACK_REPEAT_ALL", "Repeat-all submitted.", true);
        }
    case RuntimeCommandKind::PlaybackSetRepeatOne:
        {
            const auto* payload = command.payload.get<RuntimeEnabledPayload>();
            if (payload == nullptr) {
                return applied(command, "PLAYBACK_REPEAT_ONE", "Repeat-one payload missing.", false);
            }
            session_.queue().setRepeatOne(payload->enabled);
            return applied(command, "PLAYBACK_REPEAT_ONE", "Repeat-one submitted.", true);
        }
    case RuntimeCommandKind::RemoteStartActiveStream:
        return applied(command, "REMOTE_START_ACTIVE_STREAM", "Remote active stream playback submitted.", session_.remote().startActiveStream());
    case RuntimeCommandKind::EqEnable:
        session_.eq().setEnabled(true);
        return applied(command, "EQ_ENABLE", "EQ enable submitted.", true);
    case RuntimeCommandKind::EqDisable:
        session_.eq().setEnabled(false);
        return applied(command, "EQ_DISABLE", "EQ disable submitted.", true);
    case RuntimeCommandKind::EqSetBand:
        {
            const auto* payload = command.payload.get<EqSetBandPayload>();
            return applied(command, "EQ_SET_BAND", "EQ band set submitted.", payload != nullptr && session_.eq().setBand(payload->eq_band_index, payload->eq_gain_db));
        }
    case RuntimeCommandKind::EqAdjustBand:
        {
            const auto* payload = command.payload.get<EqAdjustBandPayload>();
            return applied(command, "EQ_ADJUST_BAND", "EQ band adjustment submitted.", payload != nullptr && session_.eq().adjustBand(payload->eq_band_index, payload->eq_gain_delta));
        }
    case RuntimeCommandKind::EqApplyPreset:
        {
            const auto* payload = command.payload.get<EqApplyPresetPayload>();
            return applied(command, "EQ_APPLY_PRESET", "EQ preset submitted.", payload != nullptr && session_.eq().applyPreset(payload->preset_name));
        }
    case RuntimeCommandKind::EqCyclePreset:
        {
            const auto* payload = command.payload.get<EqCyclePresetPayload>();
            return applied(command, "EQ_CYCLE_PRESET", "EQ preset cycle submitted.", payload != nullptr && session_.eq().cyclePreset(payload->preset_delta));
        }
    case RuntimeCommandKind::EqReset:
        session_.eq().reset();
        return applied(command, "EQ_RESET", "EQ reset submitted.", true);
    case RuntimeCommandKind::PlaybackStop:
    case RuntimeCommandKind::PlaybackSeek:
    case RuntimeCommandKind::QueueJump:
    case RuntimeCommandKind::QueueClear:
    case RuntimeCommandKind::RemoteReconnect:
    case RuntimeCommandKind::SettingsApplyLive:
    case RuntimeCommandKind::RuntimeShutdown:
    case RuntimeCommandKind::RuntimeReload:
        return rejectUnsupported(command, "Runtime command is part of the transport-neutral contract but is not implemented in the first in-process scope.");
    }
    return rejectUnsupported(command, "Unknown runtime command.");
}

std::uint64_t RuntimeCommandDispatcher::version() const noexcept
{
    return version_;
}

RuntimeCommandResult RuntimeCommandDispatcher::rejectUnsupported(const RuntimeCommand& command, const char* message) const
{
    return RuntimeCommandResult::reject("UNSUPPORTED_RUNTIME_COMMAND", message, command.origin, command.correlation_id, version_, version_);
}

RuntimeCommandResult RuntimeCommandDispatcher::applied(const RuntimeCommand& command, const char* code, const char* message, bool applied)
{
    const auto version_before_apply = version_;
    if (applied) {
        ++version_;
    }
    return RuntimeCommandResult::ok(code, message, command.origin, command.correlation_id, version_before_apply, version_, applied);
}

} // namespace lofibox::runtime
