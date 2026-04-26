// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/playback_visualization_source.h"

namespace lofibox::app {

AudioVisualizationFrame PlaybackVisualizationSource::currentFrame(const PlaybackBackendController& backend_controller) const
{
    return backend_controller.visualizationFrame();
}

} // namespace lofibox::app

