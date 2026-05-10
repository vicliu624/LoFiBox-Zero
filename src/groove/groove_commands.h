// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

namespace lofibox::groove {

enum class PocketGrooveCommandType {
    EnterGroove,
    ExitGroove,

    PlayPause,
    Stop,

    SetBpm,
    SetSwing,

    SelectPattern,
    SelectTrack,
    SelectStep,
    ToggleStep,

    SetStepVelocity,
    SetStepPitch,
    SetStepGain,
    SetStepSlice,

    SelectSoundSlot,
    TriggerSoundSlot,
    AssignSoundToSlot,
    EditSoundSlot,

    CaptureFromCurrentTrack,
    CaptureFromFile,
    TrimSampleStart,
    TrimSampleEnd,
    NormalizeSample,
    ReverseSample,
    AutoSliceSample,
    AssignSliceToStep,

    TriggerPunchFx,
    ReleasePunchFx,
    RecordPunchFxToStep,

    AddSongChainItem,
    RemoveSongChainItem,
    SetSongChainPattern,
    SetSongChainRepeats,
    PlaySongChain,

    SetMidiClockMode,
    SetMidiInputChannel,
    SetMidiOutputChannel,

    ExportWav,

    SaveProject,
    LoadProject,
    NewProject,
    DeleteProject
};

struct PocketGrooveCommand {
    PocketGrooveCommandType type{PocketGrooveCommandType::EnterGroove};
    std::uint8_t patternIndex{0};
    std::uint8_t trackIndex{0};
    std::uint8_t stepIndex{0};
    std::uint8_t soundSlot{0};
    std::uint8_t sliceIndex{0};
    std::uint8_t fxType{0};
    int intValue{0};
    float floatValue{0.0f};
    double doubleValue{0.0};
    bool flag{false};
    std::string text{};
};

[[nodiscard]] PocketGrooveCommand makeGrooveCommand(PocketGrooveCommandType type) noexcept;

} // namespace lofibox::groove
