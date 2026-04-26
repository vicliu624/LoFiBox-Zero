// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/canvas.h"
#include "ui/ui_models.h"

namespace lofibox::ui::effects {

void renderLyricsSideFoamSpectrum(core::Canvas& canvas, const SpectrumFrame& frame, double elapsed_seconds);

} // namespace lofibox::ui::effects
