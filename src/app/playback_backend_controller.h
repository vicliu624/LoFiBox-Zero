// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>

#include "app/runtime_services.h"

namespace lofibox::app {

class PlaybackBackendController {
public:
    void setServices(RuntimeServices* services) noexcept;

    [[nodiscard]] bool startFile(const std::filesystem::path& path, double start_seconds);
    void pause() noexcept;
    void resume() noexcept;
    void stop() noexcept;
    [[nodiscard]] AudioPlaybackState state() const;
    [[nodiscard]] AudioVisualizationFrame visualizationFrame() const;

private:
    [[nodiscard]] AudioPlaybackBackend* backend() const noexcept;

    RuntimeServices* services_{nullptr};
};

} // namespace lofibox::app
