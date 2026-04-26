// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_services_factory.h"

#include <utility>

#include "platform/host/host_runtime_service_providers.h"

namespace lofibox::platform::host {

app::RuntimeServices createHostRuntimeServices()
{
    auto context = createHostRuntimeServiceContext();
    app::RuntimeServices services{};
    services.connectivity = createHostConnectivityServices(context);
    services.metadata = createHostMetadataServices(context);
    services.playback = createHostPlaybackServices(context);
    services.remote = createHostRemoteMediaServices(context);
    services.cache = createHostCacheServices(context);
    return app::withNullRuntimeServices(std::move(services));
}

} // namespace lofibox::platform::host
