// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/offline_groove_renderer.h"

namespace lofibox::audio::groove {

SampleBuffer OfflineGrooveRenderer::render(
    const lofibox::groove::GrooveProject& project,
    const GrooveSampleBank& samples) const
{
    if (project.exportSettings.target == lofibox::groove::GrooveExportTarget::CurrentPattern) {
        return engine_.renderPattern(project, project.activePattern, samples, project.exportSettings);
    }
    return engine_.renderSongChain(project, samples, project.exportSettings);
}

} // namespace lofibox::audio::groove
