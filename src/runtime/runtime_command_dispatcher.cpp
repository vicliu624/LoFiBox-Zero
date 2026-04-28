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
        return applied(command, "PLAYBACK_PLAY", "Playback play submitted.", session_.playFirstAvailable());
    case RuntimeCommandKind::PlaybackPause:
        session_.pause();
        return applied(command, "PLAYBACK_PAUSE", "Playback pause submitted.", true);
    case RuntimeCommandKind::PlaybackResume:
        session_.resume();
        return applied(command, "PLAYBACK_RESUME", "Playback resume submitted.", true);
    case RuntimeCommandKind::PlaybackToggle:
        session_.togglePlayPause();
        return applied(command, "PLAYBACK_TOGGLE", "Playback toggle submitted.", true);
    case RuntimeCommandKind::PlaybackStartTrack:
        return applied(command, "PLAYBACK_START_TRACK", "Playback start-track submitted.", session_.startTrack(command.payload.track_id));
    case RuntimeCommandKind::QueueStep:
        return applied(command, "QUEUE_STEP", "Queue step submitted.", session_.stepQueue(command.payload.queue_delta));
    case RuntimeCommandKind::PlaybackToggleShuffle:
        session_.toggleShuffle();
        return applied(command, "PLAYBACK_SHUFFLE", "Playback shuffle submitted.", true);
    case RuntimeCommandKind::PlaybackCycleRepeat:
        session_.cycleRepeatMode();
        return applied(command, "PLAYBACK_REPEAT", "Playback repeat mode submitted.", true);
    case RuntimeCommandKind::PlaybackCycleMainMenuMode:
        session_.cycleMainMenuPlaybackMode();
        return applied(command, "PLAYBACK_MENU_MODE", "Main-menu playback mode submitted.", true);
    case RuntimeCommandKind::PlaybackSetRepeatAll:
        session_.setRepeatAll(command.payload.enabled);
        return applied(command, "PLAYBACK_REPEAT_ALL", "Repeat-all submitted.", true);
    case RuntimeCommandKind::PlaybackSetRepeatOne:
        session_.setRepeatOne(command.payload.enabled);
        return applied(command, "PLAYBACK_REPEAT_ONE", "Repeat-one submitted.", true);
    case RuntimeCommandKind::RemoteStartActiveStream:
        return applied(command, "REMOTE_START_ACTIVE_STREAM", "Remote active stream playback submitted.", session_.startActiveRemoteStream());
    case RuntimeCommandKind::EqEnable:
        session_.setEqEnabled(true);
        return applied(command, "EQ_ENABLE", "EQ enable submitted.", true);
    case RuntimeCommandKind::EqDisable:
        session_.setEqEnabled(false);
        return applied(command, "EQ_DISABLE", "EQ disable submitted.", true);
    case RuntimeCommandKind::EqSetBand:
        return applied(command, "EQ_SET_BAND", "EQ band set submitted.", session_.setEqBand(command.payload.eq_band_index, command.payload.eq_gain_db));
    case RuntimeCommandKind::EqAdjustBand:
        return applied(command, "EQ_ADJUST_BAND", "EQ band adjustment submitted.", session_.adjustEqBand(command.payload.eq_band_index, command.payload.eq_gain_delta));
    case RuntimeCommandKind::EqApplyPreset:
        return applied(command, "EQ_APPLY_PRESET", "EQ preset submitted.", session_.applyEqPreset(command.payload.preset_name));
    case RuntimeCommandKind::EqCyclePreset:
        return applied(command, "EQ_CYCLE_PRESET", "EQ preset cycle submitted.", session_.cycleEqPreset(command.payload.preset_delta));
    case RuntimeCommandKind::EqReset:
        session_.resetEq();
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
    return RuntimeCommandResult::reject("UNSUPPORTED_RUNTIME_COMMAND", message, command.correlation_id, version_);
}

RuntimeCommandResult RuntimeCommandDispatcher::applied(const RuntimeCommand& command, const char* code, const char* message, bool applied)
{
    if (applied) {
        ++version_;
    }
    return RuntimeCommandResult::ok(code, message, command.correlation_id, version_, applied);
}

} // namespace lofibox::runtime
