// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "groove/groove_pattern.h"
#include "groove/groove_song_chain.h"
#include "groove/groove_sound_slot.h"

namespace lofibox::groove {

enum class MidiClockMode {
    Internal,
    External,
    Send
};

struct GrooveMidiSettings {
    MidiClockMode clockMode{MidiClockMode::Internal};

    std::uint8_t inputChannel{10};
    std::uint8_t outputChannel{10};

    bool noteInputEnabled{true};
    bool clockInputEnabled{false};
    bool clockOutputEnabled{false};

    std::array<std::uint8_t, kGrooveSoundSlotCount> slotNoteMap{};
};

enum class GrooveExportTarget {
    CurrentPattern,
    SongChain
};

struct GrooveExportSettings {
    GrooveExportTarget target{GrooveExportTarget::SongChain};

    std::uint32_t sampleRate{48000};
    std::uint16_t bitDepth{16};

    bool normalize{true};
    bool includeMasterFx{true};
    bool includeTail{true};

    double tailSeconds{2.0};
};

struct GrooveProject {
    std::string id{};
    std::string name{};

    std::uint16_t bpm{90};
    std::uint8_t swing{0};

    std::uint8_t activePattern{0};

    std::array<GrooveSoundSlot, kGrooveSoundSlotCount> sounds{};
    std::array<GroovePattern, kGroovePatternCount> patterns{};

    GrooveSongChain songChain{};

    GrooveMidiSettings midi{};
    GrooveExportSettings exportSettings{};

    std::uint64_t createdAt{0};
    std::uint64_t updatedAt{0};
};

void initializeDefaultSlotNotes(GrooveMidiSettings& midi) noexcept;
[[nodiscard]] std::string patternName(std::uint8_t pattern_index);
[[nodiscard]] GrooveProject makeDefaultGrooveProject(std::string name = "Untitled Groove");

[[nodiscard]] const char* toString(MidiClockMode mode) noexcept;
[[nodiscard]] const char* toString(GrooveExportTarget target) noexcept;
[[nodiscard]] MidiClockMode midiClockModeFromString(std::string_view value) noexcept;
[[nodiscard]] GrooveExportTarget grooveExportTargetFromString(std::string_view value) noexcept;

[[nodiscard]] std::string grooveProjectToJson(const GrooveProject& project);
[[nodiscard]] GrooveProject grooveProjectFromJson(std::string_view json);

} // namespace lofibox::groove
