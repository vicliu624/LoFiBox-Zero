// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <map>

#include "groove/groove_track.h"

namespace lofibox::midi {

enum class MidiControlTarget {
    None,
    SelectedSlotGain,
    SelectedSlotPitch,
    FilterCutoff,
    FxAmount
};

struct GrooveMidiMapping {
    std::uint8_t inputChannel{10};
    std::array<std::uint8_t, lofibox::groove::kGrooveSoundSlotCount> slotNotes{};
    std::map<std::uint8_t, MidiControlTarget> ccTargets{};
};

[[nodiscard]] GrooveMidiMapping defaultGrooveMidiMapping() noexcept;
[[nodiscard]] int slotForNote(const GrooveMidiMapping& mapping, std::uint8_t note) noexcept;

} // namespace lofibox::midi
