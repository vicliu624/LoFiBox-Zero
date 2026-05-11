// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/groove_command_service.h"

#include <algorithm>
#include <utility>

#include "audio/groove/groove_export_service.h"
#include "audio/groove/media_segment_decoder.h"
#include "audio/groove/offline_groove_renderer.h"
#include "audio/groove/sample_capture_service.h"
#include "audio/groove/sample_editor.h"
#include "audio/groove/sample_loader.h"
#include "audio/groove/wav_exporter.h"

namespace lofibox::application {
namespace {

namespace audio_groove = lofibox::audio::groove;

[[nodiscard]] std::filesystem::path samplePathFromUri(std::string_view uri)
{
    constexpr std::string_view kFilePrefix{"file://"};
    if (uri.starts_with(kFilePrefix)) {
        return std::filesystem::path{std::string{uri.substr(kFilePrefix.size())}};
    }
    return std::filesystem::path{std::string{uri}};
}

[[nodiscard]] std::uint8_t safeSlot(std::uint8_t slot)
{
    return static_cast<std::uint8_t>(std::min<int>(slot, static_cast<int>(lofibox::groove::kGrooveSoundSlotCount - 1U)));
}

} // namespace

GrooveCommandService::GrooveCommandService() = default;

GrooveCommandService::GrooveCommandService(lofibox::groove::GrooveProjectRepository repository)
    : repository_(std::move(repository))
{}

const lofibox::groove::GrooveProjectRepository& GrooveCommandService::repository() const noexcept
{
    return repository_;
}

GrooveOperationResult GrooveCommandService::captureToSlot(
    lofibox::groove::GrooveProject& project,
    const GrooveCaptureOperation& operation) const
{
    if (!operation.source.available || operation.source.sourceUri.empty()) {
        return {false, "CAPTURE ERR NO TRACK", "no current local track is available"};
    }

    audio_groove::SampleCaptureRequest request{};
    request.sourceTrackId = operation.source.sourceTrackId;
    request.sourceUri = operation.source.sourceUri;
    request.startSeconds = operation.source.positionSeconds;
    request.durationSeconds = operation.durationSeconds;
    request.targetSoundSlot = safeSlot(operation.targetSoundSlot);
    request.normalize = operation.normalize;
    request.fadeInMs = operation.fadeInMs;
    request.fadeOutMs = operation.fadeOutMs;

    audio_groove::MediaSegmentDecoder decoder{};
    audio_groove::SampleCaptureService service{};
    const auto result = service.capture(request, decoder, repository_.paths().samplesDir);
    if (!result.ok) {
        return {false, "CAPTURE ERR", result.errorMessage};
    }

    auto& slot = project.sounds[request.targetSoundSlot];
    slot.type = lofibox::groove::GrooveSoundType::CapturedFromTrack;
    slot.id = result.sampleId;
    slot.name = result.displayName;
    slot.sourceUri = result.sampleUri;
    slot.mode = lofibox::groove::GroovePlaybackMode::OneShot;
    slot.startSeconds = 0.0;
    slot.endSeconds = result.durationSeconds;
    slot.fadeInMs = request.fadeInMs;
    slot.fadeOutMs = request.fadeOutMs;
    slot.normalized = request.normalize;
    return {true, "CAPTURE OK", {}, result.sampleUri, 100, result.durationSeconds};
}

GrooveOperationResult GrooveCommandService::rewriteSample(
    lofibox::groove::GrooveProject& project,
    std::uint8_t sound_slot,
    bool reverse) const
{
    const auto slot_index = safeSlot(sound_slot);
    auto& slot = project.sounds[slot_index];
    if (slot.sourceUri.empty()) {
        return {false, "NO SAMPLE", "selected slot has no sample"};
    }

    audio_groove::SampleLoader loader{};
    audio_groove::SampleEditor editor{};
    const auto loaded = loader.loadWav(samplePathFromUri(slot.sourceUri));
    if (!loaded.ok) {
        return {false, reverse ? "REVERSE ERR" : "NORM ERR", loaded.errorMessage};
    }
    const auto edited = reverse ? editor.reverse(loaded.buffer) : editor.normalize(loaded.buffer);
    if (!edited.ok) {
        return {false, reverse ? "REVERSE ERR" : "NORM ERR", edited.errorMessage};
    }

    const auto suffix = reverse ? "-reverse.wav" : "-norm.wav";
    const auto base = slot.id.empty() ? ("slot-" + std::to_string(static_cast<int>(slot_index) + 1)) : slot.id;
    const auto path = repository_.paths().samplesDir / (lofibox::groove::sanitizeProjectFileName(base) + suffix);
    audio_groove::WavExporter exporter{};
    const auto written = exporter.writePcm16(path, edited.buffer, false);
    if (!written.ok) {
        return {false, reverse ? "REVERSE ERR" : "NORM ERR", written.errorMessage, written.path};
    }

    slot.sourceUri = written.path.string();
    slot.id = written.path.stem().string();
    slot.endSeconds = edited.buffer.durationSeconds();
    if (reverse) {
        slot.slices.clear();
    } else {
        slot.normalized = true;
    }
    return {true, reverse ? "REVERSED" : "NORMALIZED", {}, written.path, 100, edited.buffer.durationSeconds()};
}

GrooveOperationResult GrooveCommandService::autoSlice(
    lofibox::groove::GrooveProject& project,
    std::uint8_t sound_slot,
    std::uint8_t max_slices) const
{
    const auto slot_index = safeSlot(sound_slot);
    auto& slot = project.sounds[slot_index];
    if (slot.sourceUri.empty()) {
        return {false, "SLICE ERR", "selected slot has no sample"};
    }

    audio_groove::SampleLoader loader{};
    audio_groove::SampleEditor editor{};
    const auto loaded = loader.loadWav(samplePathFromUri(slot.sourceUri));
    if (!loaded.ok) {
        return {false, "SLICE ERR", loaded.errorMessage};
    }
    slot.slices = editor.autoSlice(loaded.buffer, std::max<std::uint8_t>(1, max_slices));
    slot.mode = lofibox::groove::GroovePlaybackMode::Slice;
    return {true, "SLICED", {}, {}, 100, loaded.buffer.durationSeconds()};
}

GrooveOperationResult GrooveCommandService::exportProject(const lofibox::groove::GrooveProject& project) const
{
    audio_groove::GrooveExportService service{};
    const auto result = service.exportProject(project, repository_.paths());
    if (!result.ok) {
        return {false, "EXPORT ERR", result.errorMessage, result.path, result.progressPercent, result.durationSeconds};
    }
    const auto status = result.warningMessage.empty() ? std::string{"EXPORT DONE"} : result.warningMessage;
    return {true, status, {}, result.path, result.progressPercent, result.durationSeconds};
}

GrooveOperationResult GrooveCommandService::renderPreview(const lofibox::groove::GrooveProject& project) const
{
    audio_groove::GrooveExportService service{};
    const auto bank = service.buildSampleBank(project);
    audio_groove::OfflineGrooveRenderer renderer{};
    const auto buffer = renderer.render(project, bank);
    const auto preview_path = repository_.paths().cacheDir / "preview.wav";
    audio_groove::WavExporter exporter{};
    const auto result = exporter.writePcm16(preview_path, buffer, false);
    if (!result.ok) {
        return {false, "PLAY RENDER ERR", result.errorMessage, result.path};
    }
    return {true, "PREVIEW RENDERED", {}, result.path, 100, buffer.durationSeconds()};
}

GrooveOperationResult GrooveCommandService::saveProject(const lofibox::groove::GrooveProject& project) const
{
    std::string error{};
    if (!repository_.save(project, &error)) {
        return {false, "SAVE ERR", error};
    }
    return {true, "SAVED"};
}

GrooveOperationResult GrooveCommandService::loadFirstProject(lofibox::groove::GrooveProject& project) const
{
    const auto files = repository_.listProjectFiles();
    if (files.empty()) {
        return {false, "NO PROJECTS", "no saved groove projects"};
    }
    std::string error{};
    project = repository_.load(files.front().stem().string(), &error);
    if (!error.empty()) {
        return {false, "LOAD ERR", error};
    }
    return {true, "LOADED", {}, files.front()};
}

GrooveOperationResult GrooveCommandService::newProject(lofibox::groove::GrooveProject& project) const
{
    project = lofibox::groove::makeDefaultGrooveProject();
    return {true, "NEW PROJECT"};
}

GrooveOperationResult GrooveCommandService::deleteProject(lofibox::groove::GrooveProject& project) const
{
    std::string error{};
    if (!repository_.remove(project.id, &error)) {
        return {false, "DELETE ERR", error};
    }
    project = lofibox::groove::makeDefaultGrooveProject();
    return {true, "DELETED"};
}

} // namespace lofibox::application
