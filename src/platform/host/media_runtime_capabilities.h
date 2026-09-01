// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace lofibox::platform::host {

// Helper availability is a process-environment property. Resolve it once and
// reuse the result across metadata, artwork, lyrics and playback services.
struct MediaRuntimeCapabilities {
    std::optional<std::filesystem::path> ffmpeg{};
    std::optional<std::filesystem::path> ffprobe{};
    std::optional<std::filesystem::path> paplay{};
    std::optional<std::filesystem::path> aplay{};
};

[[nodiscard]] const MediaRuntimeCapabilities& mediaRuntimeCapabilities();

// The TDVP widget is a complete local music player, not a presentation-only
// fallback. It needs the decoder, metadata probe and one native PCM sink
// before it opens its Wayland surface.
[[nodiscard]] std::vector<std::string> missingWidgetMediaRuntimeTools(
    const MediaRuntimeCapabilities& capabilities);
void requireWidgetMediaRuntime();

} // namespace lofibox::platform::host
