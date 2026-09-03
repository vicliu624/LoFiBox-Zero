// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>

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

} // namespace lofibox::platform::host
