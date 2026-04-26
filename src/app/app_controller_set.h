// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/library_controller.h"
#include "app/playback_controller.h"
#include "app/runtime_services.h"

namespace lofibox::app {

struct AppControllerSet {
    LibraryController library{};
    PlaybackController playback{};

    void bindServices(const RuntimeServices& services)
    {
        playback.setServices(services);
    }
};

} // namespace lofibox::app
