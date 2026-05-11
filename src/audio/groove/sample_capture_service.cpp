// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/sample_capture_service.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include "audio/groove/sample_editor.h"
#include "audio/groove/wav_exporter.h"

namespace lofibox::audio::groove {

SampleCaptureResult SampleCaptureService::capture(
    const SampleCaptureRequest& request,
    const SampleSegmentDecoder& decoder,
    const std::filesystem::path& sample_dir) const
{
    if (request.sourceUri.empty()) {
        return {false, {}, {}, {}, 0.0, "source URI is empty"};
    }
    if (request.durationSeconds <= 0.0) {
        return {false, {}, {}, {}, 0.0, "capture duration must be positive"};
    }
    auto decoded = decoder.decodeSegment(request.sourceUri, request.startSeconds, request.durationSeconds);
    if (!decoded.has_value()) {
        const auto detail = decoder.lastErrorMessage();
        return {false, {}, {}, {}, 0.0, detail.empty() ? "decoder could not extract segment" : detail};
    }

    SampleEditor editor{};
    SampleBuffer buffer = std::move(*decoded);
    if (request.fadeInMs > 0.0) {
        auto result = editor.fadeIn(buffer, request.fadeInMs);
        if (result.ok) buffer = std::move(result.buffer);
    }
    if (request.fadeOutMs > 0.0) {
        auto result = editor.fadeOut(buffer, request.fadeOutMs);
        if (result.ok) buffer = std::move(result.buffer);
    }
    if (request.normalize) {
        auto result = editor.normalize(buffer);
        if (result.ok) buffer = std::move(result.buffer);
    }

    std::ostringstream id;
    id << "slot-" << static_cast<int>(request.targetSoundSlot + 1U) << "-" << static_cast<int>(request.startSeconds * 1000.0);
    const auto sample_id = id.str();
    const auto path = sample_dir / (sample_id + ".wav");
    WavExporter exporter{};
    const auto exported = exporter.writePcm16(path, buffer, false);
    if (!exported.ok) {
        return {false, sample_id, {}, {}, 0.0, exported.errorMessage};
    }
    return {true, sample_id, exported.path.string(), "CHOP_" + std::to_string(static_cast<int>(request.targetSoundSlot + 1U)), buffer.durationSeconds(), {}};
}

} // namespace lofibox::audio::groove
