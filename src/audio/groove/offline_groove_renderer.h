// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "audio/groove/groove_render_engine.h"

namespace lofibox::audio::groove {

class OfflineGrooveRenderer {
public:
    [[nodiscard]] SampleBuffer render(
        const lofibox::groove::GrooveProject& project,
        const GrooveSampleBank& samples) const;

private:
    GrooveRenderEngine engine_{};
};

} // namespace lofibox::audio::groove
