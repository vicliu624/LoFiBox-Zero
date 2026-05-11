// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <vector>

#include "groove/groove_commands.h"
#include "groove/groove_events.h"
#include "groove/groove_project.h"

namespace lofibox::groove {

enum class GrooveOverlay {
    None,
    Capture,
    SampleEdit,
    Slice,
    Chain,
    Fx,
    Midi,
    Export,
    Project
};

struct GrooveControllerProjection {
    const GrooveProject* project{};
    std::uint8_t selectedPattern{0};
    std::uint8_t selectedTrack{0};
    std::uint8_t selectedStep{0};
    std::uint8_t selectedSoundSlot{0};
    GrooveOverlay overlay{GrooveOverlay::None};
    bool playing{false};
    bool chainPlaying{false};
    std::uint8_t heldFx{0};
};

class GrooveController {
public:
    GrooveController();
    explicit GrooveController(GrooveProject project);

    [[nodiscard]] const GrooveProject& project() const noexcept;
    [[nodiscard]] GrooveProject& project() noexcept;
    void setProject(GrooveProject&& project);

    [[nodiscard]] GrooveControllerProjection projection() const noexcept;
    [[nodiscard]] std::vector<GrooveEvent> dispatch(const PocketGrooveCommand& command);

    void openOverlay(GrooveOverlay overlay) noexcept;
    void closeOverlay() noexcept;

private:
    [[nodiscard]] GrooveStep& selectedStep() noexcept;
    [[nodiscard]] GrooveTrack& selectedTrack() noexcept;
    [[nodiscard]] GroovePattern& activePattern() noexcept;
    [[nodiscard]] GrooveEvent selectionEvent() const;

    GrooveProject project_{};
    std::uint8_t selectedPattern_{0};
    std::uint8_t selectedTrack_{0};
    std::uint8_t selectedStep_{0};
    std::uint8_t selectedSoundSlot_{0};
    GrooveOverlay overlay_{GrooveOverlay::None};
    bool playing_{false};
    bool chainPlaying_{false};
    std::uint8_t heldFx_{0};
};

} // namespace lofibox::groove
