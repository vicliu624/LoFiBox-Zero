// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "app/runtime_services.h"

namespace lofibox::app {
inline bool onlineScrapingAllowed(const RuntimeServices& services)
{
    return services.connectivity.provider && services.connectivity.provider->connected()
        && (!services.ui.preferences || !services.ui.preferences->effective(RuntimeFeature::BatterySaver));
}
}
