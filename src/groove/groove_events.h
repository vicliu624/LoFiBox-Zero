// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

namespace lofibox::groove {

enum class GrooveEventType {
    Entered,
    Exited,
    ProjectChanged,
    PlaybackChanged,
    SelectionChanged,
    SoundTriggered,
    CaptureRequested,
    ExportRequested,
    Error
};

struct GrooveEvent {
    GrooveEventType type{GrooveEventType::ProjectChanged};
    std::uint8_t patternIndex{0};
    std::uint8_t trackIndex{0};
    std::uint8_t stepIndex{0};
    std::uint8_t soundSlot{0};
    std::string message{};
};

} // namespace lofibox::groove
