// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_sound_slot.h"

namespace lofibox::groove {

const char* toString(GrooveSoundType type) noexcept
{
    switch (type) {
    case GrooveSoundType::Empty: return "empty";
    case GrooveSoundType::BuiltinSample: return "builtin_sample";
    case GrooveSoundType::UserSample: return "user_sample";
    case GrooveSoundType::CapturedFromTrack: return "captured_from_track";
    case GrooveSoundType::RecordedInput: return "recorded_input";
    }
    return "empty";
}

const char* toString(GroovePlaybackMode mode) noexcept
{
    switch (mode) {
    case GroovePlaybackMode::OneShot: return "one_shot";
    case GroovePlaybackMode::Gate: return "gate";
    case GroovePlaybackMode::Loop: return "loop";
    case GroovePlaybackMode::Slice: return "slice";
    }
    return "one_shot";
}

GrooveSoundType grooveSoundTypeFromString(std::string_view value) noexcept
{
    if (value == "builtin_sample") return GrooveSoundType::BuiltinSample;
    if (value == "user_sample") return GrooveSoundType::UserSample;
    if (value == "captured_from_track") return GrooveSoundType::CapturedFromTrack;
    if (value == "recorded_input") return GrooveSoundType::RecordedInput;
    return GrooveSoundType::Empty;
}

GroovePlaybackMode groovePlaybackModeFromString(std::string_view value) noexcept
{
    if (value == "gate") return GroovePlaybackMode::Gate;
    if (value == "loop") return GroovePlaybackMode::Loop;
    if (value == "slice") return GroovePlaybackMode::Slice;
    return GroovePlaybackMode::OneShot;
}

} // namespace lofibox::groove
