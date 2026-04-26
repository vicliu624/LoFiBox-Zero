// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

namespace lofibox::app {

struct AudioVisualizationFrame {
    bool available{false};
    std::array<float, 10> bands{};
};

} // namespace lofibox::app
