// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_controller.h"

#include <algorithm>
#include <utility>

namespace lofibox::groove {

GrooveController::GrooveController()
    : project_(makeDefaultGrooveProject())
{}

GrooveController::GrooveController(GrooveProject project)
    : project_(std::move(project))
    , selectedPattern_(project_.activePattern)
{}

const GrooveProject& GrooveController::project() const noexcept
{
    return project_;
}

GrooveProject& GrooveController::project() noexcept
{
    return project_;
}

void GrooveController::setProject(GrooveProject&& project)
{
    project_ = std::move(project);
    selectedPattern_ = static_cast<std::uint8_t>(std::clamp<int>(project_.activePattern, 0, 15));
    selectedTrack_ = 0;
    selectedStep_ = 0;
    selectedSoundSlot_ = 0;
}

GrooveControllerProjection GrooveController::projection() const noexcept
{
    return GrooveControllerProjection{&project_, selectedPattern_, selectedTrack_, selectedStep_, selectedSoundSlot_, overlay_, playing_, chainPlaying_, heldFx_};
}

std::vector<GrooveEvent> GrooveController::dispatch(const PocketGrooveCommand& command)
{
    std::vector<GrooveEvent> events{};
    auto changed = [&] {
        events.push_back(GrooveEvent{GrooveEventType::ProjectChanged, selectedPattern_, selectedTrack_, selectedStep_, selectedSoundSlot_, {}});
    };

    switch (command.type) {
    case PocketGrooveCommandType::EnterGroove:
        events.push_back(GrooveEvent{GrooveEventType::Entered});
        break;
    case PocketGrooveCommandType::ExitGroove:
        playing_ = false;
        chainPlaying_ = false;
        overlay_ = GrooveOverlay::None;
        events.push_back(GrooveEvent{GrooveEventType::Exited});
        break;
    case PocketGrooveCommandType::PlayPause:
        playing_ = !playing_;
        events.push_back(GrooveEvent{GrooveEventType::PlaybackChanged});
        break;
    case PocketGrooveCommandType::Stop:
        playing_ = false;
        chainPlaying_ = false;
        events.push_back(GrooveEvent{GrooveEventType::PlaybackChanged});
        break;
    case PocketGrooveCommandType::SetBpm:
        project_.bpm = static_cast<std::uint16_t>(std::clamp(command.intValue, 40, 300));
        changed();
        break;
    case PocketGrooveCommandType::SetSwing:
        project_.swing = static_cast<std::uint8_t>(std::clamp(command.intValue, 0, 75));
        changed();
        break;
    case PocketGrooveCommandType::SelectPattern:
        selectedPattern_ = static_cast<std::uint8_t>(std::clamp<int>(command.patternIndex, 0, 15));
        project_.activePattern = selectedPattern_;
        events.push_back(selectionEvent());
        break;
    case PocketGrooveCommandType::SelectTrack:
        selectedTrack_ = static_cast<std::uint8_t>(std::clamp<int>(command.trackIndex, 0, 15));
        events.push_back(selectionEvent());
        break;
    case PocketGrooveCommandType::SelectStep:
        selectedStep_ = static_cast<std::uint8_t>(std::clamp<int>(command.stepIndex, 0, 15));
        events.push_back(selectionEvent());
        break;
    case PocketGrooveCommandType::ToggleStep:
        selectedStep().trigger = !selectedStep().trigger;
        changed();
        break;
    case PocketGrooveCommandType::SetStepVelocity:
        selectedStep().velocity = static_cast<std::uint8_t>(std::clamp(command.intValue, 0, 127));
        changed();
        break;
    case PocketGrooveCommandType::SetStepPitch:
        selectedStep().pitchSemitone = static_cast<std::int8_t>(std::clamp(command.intValue, -24, 24));
        changed();
        break;
    case PocketGrooveCommandType::SetStepGain:
        selectedStep().hasGainLock = true;
        selectedStep().gain = std::clamp(command.floatValue, 0.0f, 2.0f);
        changed();
        break;
    case PocketGrooveCommandType::SetStepSlice:
        selectedStep().sliceIndex = command.sliceIndex;
        changed();
        break;
    case PocketGrooveCommandType::SelectSoundSlot:
        selectedSoundSlot_ = static_cast<std::uint8_t>(std::clamp<int>(command.soundSlot, 0, 15));
        selectedTrack().soundSlot = selectedSoundSlot_;
        events.push_back(selectionEvent());
        break;
    case PocketGrooveCommandType::TriggerSoundSlot:
        events.push_back(GrooveEvent{GrooveEventType::SoundTriggered, selectedPattern_, selectedTrack_, selectedStep_, command.soundSlot, {}});
        break;
    case PocketGrooveCommandType::AssignSoundToSlot:
        if (!command.text.empty()) {
            auto& slot = project_.sounds[static_cast<std::size_t>(std::clamp<int>(command.soundSlot, 0, 15))];
            slot.type = GrooveSoundType::UserSample;
            slot.sourceUri = command.text;
            slot.name = "SLOT_" + std::to_string(static_cast<int>(command.soundSlot) + 1);
            changed();
        }
        break;
    case PocketGrooveCommandType::EditSoundSlot:
        overlay_ = GrooveOverlay::SampleEdit;
        events.push_back(selectionEvent());
        break;
    case PocketGrooveCommandType::CaptureFromCurrentTrack:
    case PocketGrooveCommandType::CaptureFromFile:
        overlay_ = GrooveOverlay::Capture;
        events.push_back(GrooveEvent{GrooveEventType::CaptureRequested, selectedPattern_, selectedTrack_, selectedStep_, command.soundSlot, command.text});
        break;
    case PocketGrooveCommandType::NormalizeSample:
        project_.sounds[selectedSoundSlot_].normalized = true;
        changed();
        break;
    case PocketGrooveCommandType::TrimSampleStart:
        project_.sounds[selectedSoundSlot_].startSeconds = std::max(0.0, command.doubleValue);
        if (project_.sounds[selectedSoundSlot_].endSeconds > 0.0) {
            project_.sounds[selectedSoundSlot_].startSeconds = std::min(
                project_.sounds[selectedSoundSlot_].startSeconds,
                std::max(0.0, project_.sounds[selectedSoundSlot_].endSeconds - 0.001));
        }
        changed();
        break;
    case PocketGrooveCommandType::TrimSampleEnd:
        project_.sounds[selectedSoundSlot_].endSeconds = std::max(project_.sounds[selectedSoundSlot_].startSeconds + 0.001, command.doubleValue);
        changed();
        break;
    case PocketGrooveCommandType::ReverseSample:
        changed();
        break;
    case PocketGrooveCommandType::AutoSliceSample:
        {
            auto& slot = project_.sounds[selectedSoundSlot_];
            const auto count = static_cast<std::uint8_t>(std::clamp(command.intValue <= 0 ? 8 : command.intValue, 1, 16));
            const double start = std::max(0.0, slot.startSeconds);
            const double end = slot.endSeconds > start ? slot.endSeconds : start + 1.0;
            const double width = (end - start) / static_cast<double>(count);
            slot.slices.clear();
            for (std::uint8_t index = 0; index < count; ++index) {
                SampleSlice slice{};
                slice.id = "slice-" + std::to_string(static_cast<int>(index) + 1);
                slice.name = "S" + (index + 1U < 10U ? std::string{"0"} : std::string{}) + std::to_string(static_cast<int>(index) + 1);
                slice.startSeconds = start + (static_cast<double>(index) * width);
                slice.endSeconds = index + 1U == count ? end : slice.startSeconds + width;
                slot.slices.push_back(std::move(slice));
            }
            slot.mode = GroovePlaybackMode::Slice;
        }
        changed();
        break;
    case PocketGrooveCommandType::AssignSliceToStep:
        selectedStep().sliceIndex = command.sliceIndex;
        changed();
        break;
    case PocketGrooveCommandType::TriggerPunchFx:
        heldFx_ = command.fxType;
        events.push_back(GrooveEvent{GrooveEventType::PlaybackChanged});
        break;
    case PocketGrooveCommandType::ReleasePunchFx:
        heldFx_ = 0;
        events.push_back(GrooveEvent{GrooveEventType::PlaybackChanged});
        break;
    case PocketGrooveCommandType::RecordPunchFxToStep:
        selectedStep().hasFxLock = true;
        selectedStep().fxType = command.fxType;
        selectedStep().fxAmount = command.floatValue <= 0.0f ? 1.0f : command.floatValue;
        changed();
        break;
    case PocketGrooveCommandType::AddSongChainItem:
        project_.songChain.items.push_back(GrooveSongChainItem{selectedPattern_, 1, false, 0, patternName(selectedPattern_)});
        project_.songChain.enabled = true;
        changed();
        break;
    case PocketGrooveCommandType::RemoveSongChainItem:
        if (!project_.songChain.items.empty()) {
            const auto index = std::min<std::size_t>(project_.songChain.currentItem, project_.songChain.items.size() - 1U);
            project_.songChain.items.erase(project_.songChain.items.begin() + static_cast<std::ptrdiff_t>(index));
            changed();
        }
        break;
    case PocketGrooveCommandType::SetSongChainPattern:
        if (!project_.songChain.items.empty()) {
            const auto index = std::min<std::size_t>(project_.songChain.currentItem, project_.songChain.items.size() - 1U);
            project_.songChain.items[index].patternIndex = static_cast<std::uint8_t>(std::clamp<int>(command.patternIndex, 0, 15));
            changed();
        }
        break;
    case PocketGrooveCommandType::SetSongChainRepeats:
        if (!project_.songChain.items.empty()) {
            const auto index = std::min<std::size_t>(project_.songChain.currentItem, project_.songChain.items.size() - 1U);
            project_.songChain.items[index].repeats = static_cast<std::uint8_t>(std::clamp(command.intValue, 1, 99));
            changed();
        }
        break;
    case PocketGrooveCommandType::PlaySongChain:
        chainPlaying_ = true;
        playing_ = true;
        events.push_back(GrooveEvent{GrooveEventType::PlaybackChanged});
        break;
    case PocketGrooveCommandType::SetMidiClockMode:
        project_.midi.clockMode = static_cast<MidiClockMode>(std::clamp(command.intValue, 0, 2));
        changed();
        break;
    case PocketGrooveCommandType::SetMidiInputChannel:
        project_.midi.inputChannel = static_cast<std::uint8_t>(std::clamp(command.intValue, 1, 16));
        changed();
        break;
    case PocketGrooveCommandType::SetMidiOutputChannel:
        project_.midi.outputChannel = static_cast<std::uint8_t>(std::clamp(command.intValue, 1, 16));
        changed();
        break;
    case PocketGrooveCommandType::ExportWav:
        overlay_ = GrooveOverlay::Export;
        events.push_back(GrooveEvent{GrooveEventType::ExportRequested, selectedPattern_, selectedTrack_, selectedStep_, selectedSoundSlot_, command.text});
        break;
    case PocketGrooveCommandType::SaveProject:
    case PocketGrooveCommandType::LoadProject:
    case PocketGrooveCommandType::NewProject:
    case PocketGrooveCommandType::DeleteProject:
        overlay_ = GrooveOverlay::Project;
        changed();
        break;
    }
    return events;
}

void GrooveController::openOverlay(GrooveOverlay overlay) noexcept
{
    overlay_ = overlay;
}

void GrooveController::closeOverlay() noexcept
{
    overlay_ = GrooveOverlay::None;
}

GrooveStep& GrooveController::selectedStep() noexcept
{
    return activePattern().tracks[selectedTrack_].steps[selectedStep_];
}

GrooveTrack& GrooveController::selectedTrack() noexcept
{
    return activePattern().tracks[selectedTrack_];
}

GroovePattern& GrooveController::activePattern() noexcept
{
    return project_.patterns[selectedPattern_];
}

GrooveEvent GrooveController::selectionEvent() const
{
    return GrooveEvent{GrooveEventType::SelectionChanged, selectedPattern_, selectedTrack_, selectedStep_, selectedSoundSlot_, {}};
}

} // namespace lofibox::groove
