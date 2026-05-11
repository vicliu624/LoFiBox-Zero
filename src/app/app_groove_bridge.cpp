// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_groove_bridge.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <utility>

#include "app/input_actions.h"
#include "groove/groove_project.h"

namespace lofibox::app {
namespace {

namespace groove_ui = lofibox::ui::pages::groove;

constexpr std::array<std::string_view, 5> kCaptureLengths{"1 BEAT", "1 BAR", "2 BARS", "4 BARS", "MANUAL"};

[[nodiscard]] std::string twoDigit(int value)
{
    return (value < 10 ? "0" : "") + std::to_string(value);
}

[[nodiscard]] std::string formatTime(double seconds)
{
    const auto total_centiseconds = static_cast<int>(std::max(0.0, seconds) * 100.0);
    const int minutes = total_centiseconds / 6000;
    const int whole_seconds = (total_centiseconds / 100) % 60;
    const int centiseconds = total_centiseconds % 100;
    return twoDigit(minutes) + ":" + twoDigit(whole_seconds) + "." + twoDigit(centiseconds);
}

[[nodiscard]] std::string formatSeconds(double seconds)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << std::max(0.0, seconds);
    return out.str();
}

[[nodiscard]] std::string slotLabel(const lofibox::groove::GrooveProject& project, std::uint8_t slot)
{
    const auto safe_slot = std::min<std::uint8_t>(slot, 15);
    const auto& sound = project.sounds[safe_slot];
    return twoDigit(static_cast<int>(safe_slot) + 1) + " " + (sound.name.empty() ? "EMPTY" : sound.name);
}

[[nodiscard]] std::string slotFooter(const lofibox::groove::GrooveProject& project, std::uint8_t slot, std::uint8_t step)
{
    const auto safe_slot = std::min<std::uint8_t>(slot, 15);
    const auto& sound = project.sounds[safe_slot];
    const auto label = "S" + twoDigit(static_cast<int>(safe_slot) + 1);
    const auto name = sound.name.empty() ? "EMPTY" : sound.name;
    const auto step_label = "STEP" + twoDigit(static_cast<int>(step) + 1);
    return label + " " + name + "  " + step_label + "  VEL100";
}

[[nodiscard]] double captureDurationSeconds(std::uint8_t length_index, std::uint16_t bpm)
{
    const double beat = 60.0 / static_cast<double>(std::max<std::uint16_t>(1, bpm));
    switch (std::min<std::uint8_t>(length_index, 4)) {
    case 0: return beat;
    case 1: return beat * 4.0;
    case 2: return beat * 8.0;
    case 3: return beat * 16.0;
    default: return beat * 4.0;
    }
}

[[nodiscard]] std::optional<char> digitKey(const InputEvent& event) noexcept
{
    const auto ch = singleAsciiText(event);
    if (!ch || *ch < '1' || *ch > '9') {
        return std::nullopt;
    }
    return *ch;
}

[[nodiscard]] std::optional<char> commandKey(const InputEvent& event) noexcept
{
    const auto ch = singleAsciiText(event);
    if (!ch) {
        return std::nullopt;
    }
    return static_cast<char>(std::toupper(static_cast<unsigned char>(*ch)));
}

[[nodiscard]] lofibox::groove::GrooveEvent statusEvent(std::string message)
{
    return lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::ProjectChanged, 0, 0, 0, 0, std::move(message)};
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
    playback_ = &playback;
    playback.pauseCurrentPlaybackForGroove();
    active_ = true;
    lastStatus_ = "READY";
    const auto ignored = controller_.dispatch(lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::EnterGroove));
    (void)ignored;
}

void AppGrooveBridge::exit()
{
    if (playback_ != nullptr) {
        playback_->stopGroovePlayback();
    }
    autoSave();
    active_ = false;
    playback_ = nullptr;
    const auto ignored = controller_.dispatch(lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::ExitGroove));
    (void)ignored;
}

void AppGrooveBridge::openCaptureOverlay(GrooveCurrentPlaybackSource source)
{
    captureSource_ = std::move(source);
    const auto projection = controller_.projection();
    captureTargetSlot_ = projection.selectedSoundSlot;
    controller_.openOverlay(lofibox::groove::GrooveOverlay::Capture);
    lastStatus_ = captureSource_ && captureSource_->available ? "CAPTURE READY" : "NO CURRENT TRACK";
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::dispatch(const lofibox::groove::PocketGrooveCommand& command)
{
    auto events = controller_.dispatch(command);
    if (command.type == lofibox::groove::PocketGrooveCommandType::PlayPause) {
        if (controller_.projection().playing) {
            playRenderedProject();
        } else if (playback_ != nullptr) {
            playback_->stopGroovePlayback();
        }
    }
    for (const auto& event : events) {
        if (event.type == lofibox::groove::GrooveEventType::ProjectChanged) {
            markDirty();
        }
    }
    return events;
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleInput(const InputEvent& event, bool fn_held)
{
    switch (controller_.projection().overlay) {
    case lofibox::groove::GrooveOverlay::Capture:
        return handleCaptureInput(event);
    case lofibox::groove::GrooveOverlay::SampleEdit:
    case lofibox::groove::GrooveOverlay::Slice:
        return handleSampleEditInput(event);
    case lofibox::groove::GrooveOverlay::Chain:
        return handleChainInput(event);
    case lofibox::groove::GrooveOverlay::Midi:
        return handleMidiInput(event);
    case lofibox::groove::GrooveOverlay::Export:
        return handleExportInput(event);
    case lofibox::groove::GrooveOverlay::Project:
        return handleProjectInput(event);
    case lofibox::groove::GrooveOverlay::Fx:
        if (mapInput(event) == UserAction::Back) {
            controller_.closeOverlay();
            return {statusEvent("FX CLOSED")};
        }
        return handleMainInput(event, fn_held);
    case lofibox::groove::GrooveOverlay::None:
        break;
    }
    return handleMainInput(event, fn_held);
}

const lofibox::groove::GrooveController& AppGrooveBridge::controller() const noexcept
{
    return controller_;
}

lofibox::groove::GrooveOverlay AppGrooveBridge::activeOverlay() const noexcept
{
    return controller_.projection().overlay;
}

lofibox::ui::pages::groove::PocketGrooveMainView AppGrooveBridge::mainView() const
{
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
    view.footer = dirty_ ? slotFooter(project, projection.selectedSoundSlot, projection.selectedStep) + "  DIRTY" : slotFooter(project, projection.selectedSoundSlot, projection.selectedStep);
    if (!lastStatus_.empty() && lastStatus_ != "READY") {
        view.footer = lastStatus_;
    }
    view.armedFx = projection.heldFx;
    for (std::size_t slot = 0; slot < project.sounds.size(); ++slot) {
        view.filledSlots[slot] = project.sounds[slot].type != lofibox::groove::GrooveSoundType::Empty;
    }
    const auto& pattern = project.patterns[projection.selectedPattern];
    const std::size_t first_track = (projection.selectedTrack / 4U) * 4U;
    const auto playhead = currentPlayheadStep();
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
            row_view.steps[step].playhead = projection.playing && step == playhead;
            row_view.steps[step].locked = source_step.hasGainLock || source_step.hasPanLock || source_step.hasFilterLock || source_step.hasFxLock;
        }
    }
    return view;
}

groove_ui::CaptureOverlayView AppGrooveBridge::captureOverlayView() const
{
    const auto& project = controller_.project();
    const auto safe_length = std::min<std::size_t>(captureLengthIndex_, kCaptureLengths.size() - 1U);
    groove_ui::CaptureOverlayView view{};
    view.position = captureSource_ ? formatTime(captureSource_->positionSeconds) : "00:00.00";
    view.length = std::string{kCaptureLengths[safe_length]};
    view.slot = slotLabel(project, captureTargetSlot_);
    view.name = "CHOP_" + twoDigit(static_cast<int>(captureTargetSlot_) + 1);
    view.selectedRow = 1;
    return view;
}

groove_ui::SampleEditOverlayView AppGrooveBridge::sampleEditOverlayView() const
{
    const auto projection = controller_.projection();
    const auto& slot = controller_.project().sounds[projection.selectedSoundSlot];
    groove_ui::SampleEditOverlayView view{};
    view.slotTitle = "EDIT SLOT " + twoDigit(static_cast<int>(projection.selectedSoundSlot) + 1);
    view.name = slot.name.empty() ? "EMPTY" : slot.name;
    view.startSeconds = slot.startSeconds;
    view.endSeconds = slot.endSeconds;
    view.gain = static_cast<int>(std::lround(slot.gain * 100.0f));
    view.pitch = static_cast<int>(std::lround(slot.pitchSemitone));
    view.selectedRow = sampleEditRow_;
    return view;
}

groove_ui::SliceOverlayView AppGrooveBridge::sliceOverlayView() const
{
    const auto projection = controller_.projection();
    const auto& slot = controller_.project().sounds[projection.selectedSoundSlot];
    groove_ui::SliceOverlayView view{};
    view.title = "SLICE SLOT " + twoDigit(static_cast<int>(projection.selectedSoundSlot) + 1);
    view.sliceCount = static_cast<std::uint8_t>(std::min<std::size_t>(slot.slices.size(), 16U));
    view.selectedSlice = std::min<std::uint8_t>(projection.project->patterns[projection.selectedPattern].tracks[projection.selectedTrack].steps[projection.selectedStep].sliceIndex, std::max<std::uint8_t>(view.sliceCount, 1U) - 1U);
    if (!slot.slices.empty()) {
        const auto& slice = slot.slices[view.selectedSlice];
        view.range = formatSeconds(slice.startSeconds) + "-" + formatSeconds(slice.endSeconds);
    } else {
        view.range = "NO SLICES";
    }
    view.assign = "STEP " + twoDigit(static_cast<int>(projection.selectedStep) + 1);
    return view;
}

groove_ui::ChainOverlayView AppGrooveBridge::chainOverlayView() const
{
    groove_ui::ChainOverlayView view{};
    const auto& project = controller_.project();
    for (const auto& item : project.songChain.items) {
        view.items.push_back(groove_ui::ChainOverlayItemView{lofibox::groove::patternName(item.patternIndex), item.repeats});
    }
    view.selectedItem = static_cast<std::uint8_t>(std::min<std::size_t>(project.songChain.currentItem, std::max<std::size_t>(view.items.size(), 1U) - 1U));
    return view;
}

groove_ui::FxOverlayView AppGrooveBridge::fxOverlayView() const
{
    return groove_ui::FxOverlayView{controller_.projection().heldFx};
}

groove_ui::MidiOverlayView AppGrooveBridge::midiOverlayView() const
{
    const auto& midi = controller_.project().midi;
    groove_ui::MidiOverlayView view{};
    view.clock = toString(midi.clockMode);
    view.input = "CH " + std::to_string(static_cast<int>(midi.inputChannel));
    view.output = midi.clockOutputEnabled ? ("CH " + std::to_string(static_cast<int>(midi.outputChannel))) : "OFF";
    view.sync = midi.clockMode == lofibox::groove::MidiClockMode::External ? "WAIT" : "---";
    view.selectedRow = midiRow_;
    return view;
}

groove_ui::ExportOverlayView AppGrooveBridge::exportOverlayView() const
{
    const auto& settings = controller_.project().exportSettings;
    groove_ui::ExportOverlayView view{};
    view.target = toString(settings.target);
    view.format = std::to_string(settings.sampleRate / 1000U) + "K / " + std::to_string(settings.bitDepth);
    view.normalize = settings.normalize;
    view.tailSeconds = settings.tailSeconds;
    view.exporting = exportProgress_ > 0 && exportProgress_ < 100;
    view.progressPercent = exportProgress_;
    view.fileName = lastExportFile_.empty() ? "READY" : std::filesystem::path{lastExportFile_}.filename().string();
    view.selectedRow = exportRow_;
    return view;
}

groove_ui::ProjectOverlayView AppGrooveBridge::projectOverlayView() const
{
    groove_ui::ProjectOverlayView view{};
    view.current = controller_.project().name.empty() ? "UNTITLED" : controller_.project().name;
    if (dirty_) {
        view.current += " *";
    }
    view.selectedAction = projectAction_;
    return view;
}

bool AppGrooveBridge::dirty() const noexcept
{
    return dirty_;
}

const std::string& AppGrooveBridge::lastStatus() const noexcept
{
    return lastStatus_;
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleMainInput(const InputEvent& event, bool fn_held)
{
    using lofibox::groove::PocketGrooveCommandType;
    auto projection = controller_.projection();
    auto command = lofibox::groove::makeGrooveCommand(PocketGrooveCommandType::SelectStep);
    bool has_command = true;

    if (const auto digit = digitKey(event)) {
        const auto fx = static_cast<std::uint8_t>(*digit - '0');
        command.type = fn_held ? PocketGrooveCommandType::RecordPunchFxToStep : PocketGrooveCommandType::TriggerPunchFx;
        command.fxType = fx;
        command.floatValue = 1.0f;
        controller_.openOverlay(lofibox::groove::GrooveOverlay::Fx);
        return dispatch(command);
    }

    if (const auto ch = commandKey(event)) {
        switch (*ch) {
        case 'C':
            controller_.openOverlay(lofibox::groove::GrooveOverlay::Capture);
            return {statusEvent("CAPTURE")};
        case 'E':
            controller_.openOverlay(lofibox::groove::GrooveOverlay::SampleEdit);
            return {statusEvent("EDIT")};
        case 'M':
            controller_.openOverlay(lofibox::groove::GrooveOverlay::Midi);
            return {statusEvent("MIDI")};
        case 'P':
            controller_.openOverlay(lofibox::groove::GrooveOverlay::Project);
            return {statusEvent("PROJECT")};
        default:
            break;
        }
    }

    switch (event.key) {
    case InputKey::F2:
        command.type = PocketGrooveCommandType::PlayPause;
        break;
    case InputKey::F3:
        command.type = PocketGrooveCommandType::Stop;
        if (playback_ != nullptr) playback_->stopGroovePlayback();
        break;
    case InputKey::F4:
        controller_.openOverlay(lofibox::groove::GrooveOverlay::Capture);
        return {statusEvent("CAPTURE")};
    case InputKey::F5:
        controller_.openOverlay(lofibox::groove::GrooveOverlay::SampleEdit);
        return {statusEvent("EDIT")};
    case InputKey::F6:
        controller_.openOverlay(lofibox::groove::GrooveOverlay::Chain);
        return {statusEvent("CHAIN")};
    case InputKey::F7:
        controller_.openOverlay(lofibox::groove::GrooveOverlay::Fx);
        return {statusEvent("FX")};
    case InputKey::F8:
        controller_.openOverlay(lofibox::groove::GrooveOverlay::Midi);
        return {statusEvent("MIDI")};
    case InputKey::F9:
        controller_.openOverlay(lofibox::groove::GrooveOverlay::Export);
        return {statusEvent("EXPORT")};
    case InputKey::F10:
        controller_.openOverlay(lofibox::groove::GrooveOverlay::Project);
        return {statusEvent("PROJECT")};
    default:
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
        case UserAction::PageUp:
            command.type = PocketGrooveCommandType::SelectPattern;
            command.patternIndex = projection.selectedPattern == 0U ? 15U : static_cast<std::uint8_t>(projection.selectedPattern - 1U);
            break;
        case UserAction::PageDown:
            command.type = PocketGrooveCommandType::SelectPattern;
            command.patternIndex = static_cast<std::uint8_t>((projection.selectedPattern + 1U) % 16U);
            break;
        case UserAction::Confirm:
            command.type = PocketGrooveCommandType::ToggleStep;
            break;
        case UserAction::Back:
        case UserAction::Home:
            exit();
            return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::Exited}};
        default:
            has_command = false;
            break;
        }
        break;
    }
    if (!has_command) {
        return {};
    }
    return dispatch(command);
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleCaptureInput(const InputEvent& event)
{
    switch (mapInput(event)) {
    case UserAction::Left:
        captureLengthIndex_ = captureLengthIndex_ == 0U ? static_cast<std::uint8_t>(kCaptureLengths.size() - 1U) : static_cast<std::uint8_t>(captureLengthIndex_ - 1U);
        return {statusEvent("LEN " + std::string{kCaptureLengths[captureLengthIndex_]})};
    case UserAction::Right:
        captureLengthIndex_ = static_cast<std::uint8_t>((captureLengthIndex_ + 1U) % kCaptureLengths.size());
        return {statusEvent("LEN " + std::string{kCaptureLengths[captureLengthIndex_]})};
    case UserAction::Up:
        captureTargetSlot_ = captureTargetSlot_ == 0U ? 15U : static_cast<std::uint8_t>(captureTargetSlot_ - 1U);
        return {statusEvent("SLOT " + slotLabel(controller_.project(), captureTargetSlot_))};
    case UserAction::Down:
        captureTargetSlot_ = static_cast<std::uint8_t>((captureTargetSlot_ + 1U) % 16U);
        return {statusEvent("SLOT " + slotLabel(controller_.project(), captureTargetSlot_))};
    case UserAction::Confirm:
        return executeCapture();
    case UserAction::Back:
        controller_.closeOverlay();
        lastStatus_ = "CAPTURE CANCEL";
        return {statusEvent(lastStatus_)};
    default:
        return {};
    }
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleSampleEditInput(const InputEvent& event)
{
    auto& slot = controller_.project().sounds[controller_.projection().selectedSoundSlot];
    switch (mapInput(event)) {
    case UserAction::Up:
        sampleEditRow_ = std::max(0, sampleEditRow_ - 1);
        return {statusEvent("EDIT ROW")};
    case UserAction::Down:
        sampleEditRow_ = std::min(5, sampleEditRow_ + 1);
        return {statusEvent("EDIT ROW")};
    case UserAction::Left:
    case UserAction::Right: {
        const double sign = mapInput(event) == UserAction::Right ? 1.0 : -1.0;
        switch (sampleEditRow_) {
        case 0:
            slot.startSeconds = std::max(0.0, slot.startSeconds + (sign * 0.01));
            if (slot.endSeconds > 0.0) slot.startSeconds = std::min(slot.startSeconds, std::max(0.0, slot.endSeconds - 0.01));
            break;
        case 1:
            slot.endSeconds = std::max(slot.startSeconds + 0.01, slot.endSeconds + (sign * 0.01));
            break;
        case 2:
            slot.gain = std::clamp(slot.gain + static_cast<float>(sign * 0.05), 0.0f, 2.0f);
            break;
        case 3:
            slot.pitchSemitone = std::clamp(slot.pitchSemitone + static_cast<float>(sign), -24.0f, 24.0f);
            break;
        case 4:
            slot.fadeInMs = std::max(0.0, slot.fadeInMs + sign);
            slot.fadeOutMs = std::max(0.0, slot.fadeOutMs + sign);
            break;
        case 5:
            controller_.openOverlay(lofibox::groove::GrooveOverlay::Slice);
            break;
        default:
            break;
        }
        markDirty();
        return {statusEvent("SAMPLE EDIT")};
    }
    case UserAction::Confirm:
        return rewriteSelectedSample(false);
    case UserAction::Back:
        controller_.closeOverlay();
        autoSave();
        return {statusEvent("EDIT DONE")};
    default:
        break;
    }

    if (const auto edit_key = commandKey(event); edit_key && *edit_key == 'R') {
        return rewriteSelectedSample(true);
    }
    if (event.key == InputKey::F6 || (commandKey(event) && *commandKey(event) == 'S')) {
        return autoSliceSelectedSample();
    }
    return {};
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleChainInput(const InputEvent& event)
{
    auto& chain = controller_.project().songChain;
    switch (mapInput(event)) {
    case UserAction::Left:
        if (!chain.items.empty()) {
            chain.currentItem = chain.currentItem == 0U ? static_cast<std::uint16_t>(chain.items.size() - 1U) : static_cast<std::uint16_t>(chain.currentItem - 1U);
        }
        return {statusEvent("CHAIN CURSOR")};
    case UserAction::Right:
        if (!chain.items.empty()) {
            chain.currentItem = static_cast<std::uint16_t>((chain.currentItem + 1U) % chain.items.size());
        }
        return {statusEvent("CHAIN CURSOR")};
    case UserAction::Up:
        if (!chain.items.empty()) {
            auto& item = chain.items[std::min<std::size_t>(chain.currentItem, chain.items.size() - 1U)];
            item.repeats = static_cast<std::uint8_t>(std::min<int>(99, item.repeats + 1));
            markDirty();
        }
        return {statusEvent("REPEAT +")};
    case UserAction::Down:
        if (!chain.items.empty()) {
            auto& item = chain.items[std::min<std::size_t>(chain.currentItem, chain.items.size() - 1U)];
            item.repeats = static_cast<std::uint8_t>(std::max<int>(1, item.repeats - 1));
            markDirty();
        }
        return {statusEvent("REPEAT -")};
    case UserAction::Confirm: {
        auto command = lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::AddSongChainItem);
        return dispatch(command);
    }
    case UserAction::Back:
        controller_.closeOverlay();
        autoSave();
        return {statusEvent("CHAIN DONE")};
    default:
        break;
    }
    if (event.key == InputKey::Delete && !chain.items.empty()) {
        return dispatch(lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::RemoveSongChainItem));
    }
    return {};
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleMidiInput(const InputEvent& event)
{
    auto& midi = controller_.project().midi;
    switch (mapInput(event)) {
    case UserAction::Up:
        midiRow_ = std::max(0, midiRow_ - 1);
        return {statusEvent("MIDI ROW")};
    case UserAction::Down:
        midiRow_ = std::min(2, midiRow_ + 1);
        return {statusEvent("MIDI ROW")};
    case UserAction::Left:
    case UserAction::Right: {
        const int sign = mapInput(event) == UserAction::Right ? 1 : -1;
        if (midiRow_ == 0) {
            const int mode = (static_cast<int>(midi.clockMode) + 3 + sign) % 3;
            midi.clockMode = static_cast<lofibox::groove::MidiClockMode>(mode);
            midi.clockInputEnabled = midi.clockMode == lofibox::groove::MidiClockMode::External;
            midi.clockOutputEnabled = midi.clockMode == lofibox::groove::MidiClockMode::Send;
        } else if (midiRow_ == 1) {
            midi.inputChannel = static_cast<std::uint8_t>(std::clamp<int>(midi.inputChannel + sign, 1, 16));
        } else {
            midi.outputChannel = static_cast<std::uint8_t>(std::clamp<int>(midi.outputChannel + sign, 1, 16));
        }
        markDirty();
        return {statusEvent("MIDI SET")};
    }
    case UserAction::Back:
        controller_.closeOverlay();
        autoSave();
        return {statusEvent("MIDI DONE")};
    default:
        return {};
    }
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleExportInput(const InputEvent& event)
{
    auto& settings = controller_.project().exportSettings;
    switch (mapInput(event)) {
    case UserAction::Up:
        exportRow_ = std::max(0, exportRow_ - 1);
        return {statusEvent("EXPORT ROW")};
    case UserAction::Down:
        exportRow_ = std::min(3, exportRow_ + 1);
        return {statusEvent("EXPORT ROW")};
    case UserAction::Left:
    case UserAction::Right:
        if (exportRow_ == 0) {
            settings.target = settings.target == lofibox::groove::GrooveExportTarget::SongChain
                ? lofibox::groove::GrooveExportTarget::CurrentPattern
                : lofibox::groove::GrooveExportTarget::SongChain;
        } else if (exportRow_ == 1) {
            settings.normalize = !settings.normalize;
        } else if (exportRow_ == 2) {
            settings.includeMasterFx = !settings.includeMasterFx;
        } else {
            const double sign = mapInput(event) == UserAction::Right ? 0.5 : -0.5;
            settings.tailSeconds = std::clamp(settings.tailSeconds + sign, 0.0, 10.0);
        }
        markDirty();
        return {statusEvent("EXPORT SET")};
    case UserAction::Confirm:
        return executeExport();
    case UserAction::Back:
        controller_.closeOverlay();
        return {statusEvent("EXPORT CLOSED")};
    default:
        return {};
    }
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::handleProjectInput(const InputEvent& event)
{
    switch (mapInput(event)) {
    case UserAction::Left:
        projectAction_ = (projectAction_ + 3) % 4;
        return {statusEvent("PROJECT ACTION")};
    case UserAction::Right:
        projectAction_ = (projectAction_ + 1) % 4;
        return {statusEvent("PROJECT ACTION")};
    case UserAction::Confirm:
        return executeProjectAction();
    case UserAction::Back:
        controller_.closeOverlay();
        return {statusEvent("PROJECT CLOSED")};
    default:
        return {};
    }
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::executeCapture()
{
    if (!captureSource_) {
        lastStatus_ = "CAPTURE ERR NO TRACK";
        return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::Error, 0, 0, 0, captureTargetSlot_, lastStatus_}};
    }
    const auto& project = controller_.project();
    const auto result = operations_.captureToSlot(
        controller_.project(),
        ::lofibox::application::GrooveCaptureOperation{
            *captureSource_,
            captureTargetSlot_,
            captureDurationSeconds(captureLengthIndex_, project.bpm),
            true,
            2.0,
            4.0});
    if (!result.ok) {
        applyOperationResult(result);
        return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::Error, 0, 0, 0, captureTargetSlot_, result.errorMessage}};
    }

    auto command = lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::SelectSoundSlot);
    command.soundSlot = captureTargetSlot_;
    (void)controller_.dispatch(command);
    markDirty();
    autoSave();
    controller_.closeOverlay();
    lastStatus_ = result.status + " " + slotLabel(controller_.project(), captureTargetSlot_);
    return {statusEvent(lastStatus_)};
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::executeExport()
{
    exportProgress_ = 5;
    lastStatus_ = "EXPORTING";
    const auto result = operations_.exportProject(controller_.project());
    if (!result.ok) {
        exportProgress_ = 0;
        applyOperationResult(result);
        return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::Error, 0, 0, 0, 0, result.errorMessage}};
    }
    exportProgress_ = result.progressPercent;
    lastExportFile_ = result.path.string();
    applyOperationResult(result);
    return {statusEvent(lastStatus_)};
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::executeProjectAction()
{
    ::lofibox::application::GrooveOperationResult result{};
    if (projectAction_ == 0) {
        result = operations_.saveProject(controller_.project());
        if (result.ok) {
            dirty_ = false;
        }
    } else if (projectAction_ == 1) {
        auto project = std::make_unique<lofibox::groove::GrooveProject>(controller_.project());
        result = operations_.loadFirstProject(*project);
        if (result.ok) {
            controller_.setProject(std::move(*project));
            dirty_ = false;
        }
    } else if (projectAction_ == 2) {
        auto project = std::make_unique<lofibox::groove::GrooveProject>();
        result = operations_.newProject(*project);
        controller_.setProject(std::move(*project));
        dirty_ = true;
    } else {
        auto project = std::make_unique<lofibox::groove::GrooveProject>(controller_.project());
        result = operations_.deleteProject(*project);
        if (result.ok) {
            controller_.setProject(std::move(*project));
            dirty_ = false;
        }
    }
    applyOperationResult(result);
    if (!result.ok && !result.errorMessage.empty()) {
        return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::Error, 0, 0, 0, 0, result.errorMessage}};
    }
    return {statusEvent(lastStatus_)};
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::rewriteSelectedSample(bool reverse)
{
    const auto projection = controller_.projection();
    const auto result = operations_.rewriteSample(controller_.project(), projection.selectedSoundSlot, reverse);
    applyOperationResult(result);
    if (!result.ok) {
        return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::Error, 0, 0, 0, projection.selectedSoundSlot, result.errorMessage}};
    }
    markDirty();
    autoSave();
    return {statusEvent(lastStatus_)};
}

std::vector<lofibox::groove::GrooveEvent> AppGrooveBridge::autoSliceSelectedSample()
{
    const auto projection = controller_.projection();
    const auto result = operations_.autoSlice(controller_.project(), projection.selectedSoundSlot, 16);
    applyOperationResult(result);
    if (!result.ok) {
        return {lofibox::groove::GrooveEvent{lofibox::groove::GrooveEventType::Error, 0, 0, 0, projection.selectedSoundSlot, result.errorMessage}};
    }
    markDirty();
    autoSave();
    return {statusEvent(lastStatus_)};
}

void AppGrooveBridge::applyOperationResult(const ::lofibox::application::GrooveOperationResult& result)
{
    if (!result.ok && !result.errorMessage.empty()) {
        lastStatus_ = result.errorMessage;
    } else if (!result.status.empty()) {
        lastStatus_ = result.status;
    } else if (!result.errorMessage.empty()) {
        lastStatus_ = result.errorMessage;
    }
}

void AppGrooveBridge::markDirty()
{
    dirty_ = true;
}

void AppGrooveBridge::autoSave()
{
    if (!dirty_) {
        return;
    }
    const auto result = operations_.saveProject(controller_.project());
    if (result.ok) {
        dirty_ = false;
        lastStatus_ = "AUTO SAVED";
    } else if (!result.errorMessage.empty()) {
        lastStatus_ = result.status.empty() ? "SAVE ERR" : result.status;
    }
}

void AppGrooveBridge::playRenderedProject()
{
    if (playback_ == nullptr) {
        return;
    }
    const auto result = operations_.renderPreview(controller_.project());
    if (!result.ok) {
        applyOperationResult(result);
        return;
    }
    if (playback_->playGrooveRenderFile(result.path)) {
        playStarted_ = clock::now();
        lastStatus_ = "PREVIEW PLAY";
    } else {
        lastStatus_ = "PLAYBACK ERR";
    }
}

std::uint8_t AppGrooveBridge::currentPlayheadStep() const noexcept
{
    if (!controller_.projection().playing || playStarted_ == clock::time_point{}) {
        return controller_.projection().selectedStep;
    }
    const auto& project = controller_.project();
    const double beat = 60.0 / static_cast<double>(std::max<std::uint16_t>(1, project.bpm));
    const double step_seconds = beat / 4.0;
    if (step_seconds <= 0.0) {
        return 0;
    }
    const double elapsed = std::chrono::duration<double>(clock::now() - playStarted_).count();
    return static_cast<std::uint8_t>(static_cast<int>(elapsed / step_seconds) % 16);
}

} // namespace lofibox::app
