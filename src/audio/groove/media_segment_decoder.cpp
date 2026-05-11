// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/media_segment_decoder.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "audio/groove/sample_editor.h"
#include "audio/groove/sample_loader.h"

namespace lofibox::audio::groove {
namespace {

[[nodiscard]] std::filesystem::path pathFromUri(std::string_view uri)
{
    constexpr std::string_view kFilePrefix{"file://"};
    if (uri.starts_with(kFilePrefix)) {
        return std::filesystem::path{std::string{uri.substr(kFilePrefix.size())}};
    }
    return std::filesystem::path{std::string{uri}};
}

[[nodiscard]] std::string lowerExtension(const std::filesystem::path& path)
{
    auto ext = path.extension().string();
    for (char& ch : ext) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return ext;
}

} // namespace

std::optional<SampleBuffer> MediaSegmentDecoder::decodeSegment(
    std::string_view source_uri,
    double start_seconds,
    double duration_seconds) const
{
    lastError_.clear();
    const auto path = pathFromUri(source_uri);
    const auto ext = lowerExtension(path);
    if (ext != ".wav") {
        lastError_ = "CAPTURE FORMAT UNSUPPORTED";
        return std::nullopt;
    }

    SampleLoader loader{};
    const auto loaded = loader.loadWav(path);
    if (!loaded.ok) {
        lastError_ = loaded.errorMessage.empty() ? "CAPTURE DECODE FAILED" : loaded.errorMessage;
        return std::nullopt;
    }

    SampleEditor editor{};
    const auto trimmed = editor.trim(loaded.buffer, start_seconds, start_seconds + duration_seconds);
    if (!trimmed.ok) {
        lastError_ = trimmed.errorMessage.empty() ? "CAPTURE RANGE INVALID" : trimmed.errorMessage;
        return std::nullopt;
    }
    return trimmed.buffer;
}

std::string MediaSegmentDecoder::lastErrorMessage() const
{
    return lastError_;
}

} // namespace lofibox::audio::groove
