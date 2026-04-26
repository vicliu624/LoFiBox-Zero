// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>

#include "app/audio_visualization.h"
#include "app/runtime_services.h"

namespace lofibox::audio {

class AudioPipelineController {
public:
    void bind(app::RuntimeServices* services) noexcept;

    [[nodiscard]] bool startFile(const std::filesystem::path& path, double start_seconds);
    void pause() noexcept;
    void resume() noexcept;
    void stop() noexcept;

    [[nodiscard]] app::AudioPlaybackState state() const;
    [[nodiscard]] app::AudioVisualizationFrame visualizationFrame() const;

private:
    [[nodiscard]] app::AudioPlaybackBackend* backend() const noexcept;

    app::RuntimeServices* services_{nullptr};
};

} // namespace lofibox::audio
