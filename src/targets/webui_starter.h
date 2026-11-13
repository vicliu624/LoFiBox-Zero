// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "app/lofibox_app.h"
#include "platform/host/library_enrich_provider_adapter.h"
#include "platform/host/library_query_provider_adapter.h"
#include "platform/host/runtime_paths.h"
#include "webui/webui_config.h"
#include "webui/webui_runtime_adapter.h"
#include "webui/webui_server.h"

namespace lofibox::targets {
// One lifecycle implementation for framebuffer and desktop targets. All
// providers, client workers and listening sockets are released when disabled.
inline app::AppStarter makeWebUiStarter(webui::WebUiConfig config, app::RuntimeServices& services)
{
    services.ui.preferences->configureWebUiDefault(config.enabled);
    services.ui.webui_url = webui::displayUrlForWebUi(config);
    config.enabled = true;
    struct Context {
        std::unique_ptr<platform::host::LibraryQueryProviderAdapter> library;
        std::unique_ptr<platform::host::LibraryEnrichProviderAdapter> enrich;
        std::unique_ptr<webui::WebUiRuntimeAdapter> adapter;
        std::unique_ptr<webui::WebUiServer> server;
    };
    auto context = std::make_shared<Context>();
    return [config, context, preferences = services.ui.preferences,
            artwork = services.metadata.artwork_provider, cache = services.cache.cache_manager,
            connectivity = services.connectivity.provider,
            theme = services.ui.theme](runtime::RuntimeCommandClient& client,
                                      const application::AppServiceRegistry& registry) {
        if (!preferences->effective(app::RuntimeFeature::WebUi)) {
            context->server.reset();
            context->adapter.reset();
            context->enrich.reset();
            context->library.reset();
            return;
        }
        if (context->server && context->server->isRunning()) return;
        context->server.reset();
        context->adapter.reset();
        context->enrich.reset();
        context->library.reset();
        context->library = std::make_unique<platform::host::LibraryQueryProviderAdapter>(
            artwork, platform::host::runtime_paths::appCacheDir());
        context->library->bind(registry.libraryQueries());
        context->enrich = std::make_unique<platform::host::LibraryEnrichProviderAdapter>(cache, connectivity, preferences);
        context->adapter = std::make_unique<webui::WebUiRuntimeAdapter>(client);
        context->server = std::make_unique<webui::WebUiServer>(config, *context->adapter);
        context->server->setLibraryQueryProvider(context->library.get());
        context->server->setLibraryEnrichProvider(context->enrich.get());
        context->server->setTheme(theme.get());
        (void)context->server->start();
    };
}
}
