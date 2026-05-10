// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_groove_bridge.h"

#include <algorithm>
#include <utility>

#include "app/input_actions.h"
#include "groove/groove_project.h"

namespace lofibox::app {
namespace {

[[nodiscard]] std::string slotFooter(const lofibox::groove::GrooveProject& project, std::uint8_t slot, std::uint8_t step)
{
    const auto safe_slot = std::min<std::uint8_t>(slot, 15);
    const auto& sound = project.sounds[safe_slot];
    const auto label = "S" + (safe_slot + 1U < 10U ? std::string{"0"} : std::string{}) + std::to_string(static_cast<int>(safe_slot + 1U));
    const auto name = sound.name.empty() ? "EMPTY" : sound.name;
    const auto step_label = "STEP" + (step + 1U < 10U ? std::string{"0"} : std::string{}) + std::to_string(static_cast<int>(step + 1U));
    return label + " " + name + "  " + step_label + "  VEL100";
}

} // namespace

AppGrooveBridge::AppGrooveBridge() = default;

AppGrooveBridge::AppGrooveBridge(lofibox::groove::GrooveProject project)
    : controller_(std::move(project))
{}

bool AppGrooveBridge::active() const noexcept
{
    return active_;
}

void AppGrooveBridge::enter(GroovePlaybackControl& playback)
{
    playback.pauseCurrentPlaybackForGroove();
    active_ = true;
    const auto ignored = controller_.dispatch(lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::EnterGroove));
    (void)ignored;
}

void AppGrooveBridge::exit()
{
    active_ = false;
    const auto ignored = controller_.dispatch(lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::ExitGroove));
    (void)ignored;
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::dispatch(const lofibox::groove::PocketGrooveCommand& command)
{
    return controller_.dispatch(command);
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleInput(const InputEvent& event, bool fn_held)
{
    using lofibox::groove::PocketGrooveCommandType;
    auto projection = controller_.projection();
    auto command = lofibox::groove::makeGrooveCommand(PocketGrooveCommandType::SelectStep);
    bool has_command = true;

    switch (mapInput(event)) {
    case UserAction::Left:
        if (fn_held) {
            command.type = PocketGrooveCommandType::SelectPattern;
            command.patternIndex = projection.selectedPattern == 0U ? 15U : static_cast<std::uint8_t>(projection.selectedPattern - 1U);
        } else {
            command.stepIndex = projection.selectedStep == 0U ? 15U : static_cast<std::uint8_t>(projection.selectedStep - 1U);
        }
        break;
    case UserAction::Right:
        if (fn_held) {
            command.type = PocketGrooveCommandType::SelectPattern;
            command.patternIndex = static_cast<std::uint8_t>((projection.selectedPattern + 1U) % 16U);
        } else {
            command.stepIndex = static_cast<std::uint8_t>((projection.selectedStep + 1U) % 16U);
        }
        break;
    case UserAction::Up:
        if (fn_held) {
            command.type = PocketGrooveCommandType::SelectSoundSlot;
            command.soundSlot = projection.selectedSoundSlot == 0U ? 15U : static_cast<std::uint8_t>(projection.selectedSoundSlot - 1U);
        } else {
            command.type = PocketGrooveCommandType::SelectTrack;
            command.trackIndex = projection.selectedTrack == 0U ? 15U : static_cast<std::uint8_t>(projection.selectedTrack - 1U);
        }
        break;
    case UserAction::Down:
        if (fn_held) {
            command.type = PocketGrooveCommandType::SelectSoundSlot;
            command.soundSlot = static_cast<std::uint8_t>((projection.selectedSoundSlot + 1U) % 16U);
        } else {
            command.type = PocketGrooveCommandType::SelectTrack;
            command.trackIndex = static_cast<std::uint8_t>((projection.selectedTrack + 1U) % 16U);
        }
        break;
    case UserAction::Confirm:
        command.type = PocketGrooveCommandType::ToggleStep;
        break;
    case UserAction::Back:
        if (projection.overlay != lofibox::groove::GrooveOverlay::None) {
            controller_.closeOverlay();
            return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::SelectionChanged}};
        }
        exit();
        return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::Exited}};
    default:
        has_command = false;
        break;
    }
    if (!has_command) {
        return {};
    }
    return controller_.dispatch(command);
}

const lofibox::groove::GrooveController& AppGrooveBridge::controller() const noexcept
{
    return controller_;
}

lofibox::ui::pages::groove::PocketGrooveMainView AppGrooveBridge::mainView() const
{
    namespace groove_ui = lofibox::ui::pages::groove;
    const auto projection = controller_.projection();
    const auto& project = *projection.project;
    groove_ui::PocketGrooveMainView view{};
    view.patternName = project.patterns[projection.selectedPattern].name;
    view.bpm = project.bpm;
    view.playing = projection.playing;
    view.chainEnabled = project.songChain.enabled;
    view.selectedSlot = projection.selectedSoundSlot;
    view.selectedStep = projection.selectedStep;
    view.selectedTrack = projection.selectedTrack;
    view.footer = slotFooter(project, projection.selectedSoundSlot, projection.selectedStep);
    view.heldFx = projection.heldFx;
    for (std::size_t slot = 0; slot < project.sounds.size(); ++slot) {
        view.filledSlots[slot] = project.sounds[slot].type != lofibox::groove::GrooveSoundType::Empty;
    }
    const auto& pattern = project.patterns[projection.selectedPattern];
    const std::size_t first_track = (projection.selectedTrack / 4U) * 4U;
    for (std::size_t row = 0; row < view.visibleTracks.size(); ++row) {
        const std::size_t track_index = std::min<std::size_t>(first_track + row, pattern.tracks.size() - 1U);
        auto& row_view = view.visibleTracks[row];
        row_view.label = project.sounds[pattern.tracks[track_index].soundSlot].name.empty()
            ? ("T" + std::to_string(track_index + 1U))
            : project.sounds[pattern.tracks[track_index].soundSlot].name;
        for (std::size_t step = 0; step < row_view.steps.size(); ++step) {
            const auto& source_step = pattern.tracks[track_index].steps[step];
            row_view.steps[step].trigger = source_step.trigger;
            row_view.steps[step].selected = track_index == projection.selectedTrack && step == projection.selectedStep;
            row_view.steps[step].locked = source_step.hasGainLock || source_step.hasPanLock || source_step.hasFilterLock || source_step.hasFxLock;
        }
    }
    return view;
}

} // namespace lofibox::app
