// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/audio_visualization.h"
#include "app/playback_backend_controller.h"

namespace lofibox::app {

class PlaybackVisualizationSource {
public:
    [[nodiscard]] AudioVisualizationFrame currentFrame(const PlaybackBackendController& backend_controller) const;
};

} // namespace lofibox::app
