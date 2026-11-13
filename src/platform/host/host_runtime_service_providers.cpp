// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/host_runtime_service_providers.h"

#include <memory>
#include <utility>

#include "platform/host/runtime_provider_factories.h"
#include "platform/host/runtime_paths.h"
#include "platform/host/xdg_remote_profile_store.h"
#include "platform/host/runtime_preferences_store.h"

namespace lofibox::platform::host {
namespace {
class ScrapingConnectivity final : public app::ConnectivityProvider {
public:
    ScrapingConnectivity(std::shared_ptr<app::ConnectivityProvider> network,
                         std::shared_ptr<app::RuntimePreferences> preferences)
        : network_(std::move(network)), preferences_(std::move(preferences)) {}
    bool connected() const override {
        return network_ && network_->connected()
            && (!preferences_ || !preferences_->effective(app::RuntimeFeature::BatterySaver));
    }
    std::string displayName() const override { return network_ ? network_->displayName() : "OFFLINE"; }
private:
    std::shared_ptr<app::ConnectivityProvider> network_;
    std::shared_ptr<app::RuntimePreferences> preferences_;
};
}

HostRuntimeServiceContext createHostRuntimeServiceContext()
{
    HostRuntimeServiceContext context{};
    context.preferences = loadRuntimePreferences(runtime_paths::appConfigDir() / "runtime-preferences.conf");
    context.cache = std::make_shared<runtime_detail::SharedRuntimeCache>();
    return context;
}

app::ConnectivityServices createHostConnectivityServices(HostRuntimeServiceContext& context)
{
    context.connectivity.provider = createHostConnectivityProvider();
    return context.connectivity;
}

app::MetadataServices createHostMetadataServices(HostRuntimeServiceContext& context)
{
    if (!context.connectivity.provider) {
        context.connectivity = createHostConnectivityServices(context);
    }
    if (!context.tag_writer) {
        context.tag_writer = createHostTagWriter();
    }
    const auto scraping = std::make_shared<ScrapingConnectivity>(context.connectivity.provider, context.preferences);
    if (!context.track_identity_provider) {
        context.track_identity_provider = createHostTrackIdentityProvider(context.cache, scraping);
    }

    app::MetadataServices metadata_group{};
    metadata_group.metadata_provider = createHostMetadataProvider(
        context.cache,
        scraping,
        context.tag_writer,
        context.track_identity_provider);
    metadata_group.track_identity_provider = context.track_identity_provider;
    metadata_group.artwork_provider = createHostArtworkProvider(
        context.cache,
        scraping,
        context.tag_writer);
    metadata_group.lyrics_provider = createHostLyricsProvider(
        context.cache,
        scraping,
        context.tag_writer,
        context.track_identity_provider);
    metadata_group.tag_writer = context.tag_writer;
    return metadata_group;
}

app::PlaybackServices createHostPlaybackServices(HostRuntimeServiceContext& context)
{
    app::PlaybackServices playback_group{};
    playback_group.audio_backend = createHostAudioPlaybackBackend(context.preferences);
    return playback_group;
}

app::RemoteMediaServices createHostRemoteMediaServices(HostRuntimeServiceContext&)
{
    app::RemoteMediaServices remote_group{};
    remote_group.remote_source_provider = createHostRemoteSourceProvider();
    remote_group.remote_catalog_provider = createHostRemoteCatalogProvider();
    remote_group.remote_stream_resolver = createHostRemoteStreamResolver();
    remote_group.remote_profile_store = std::make_shared<XdgRemoteProfileStore>();
    return remote_group;
}

app::CacheServices createHostCacheServices(HostRuntimeServiceContext& context)
{
    if (!context.cache_services.cache_manager) {
        context.cache_services.cache_manager = std::make_shared<::lofibox::cache::CacheManager>(runtime_paths::appCacheDir());
    }
    return context.cache_services;
}

} // namespace lofibox::platform::host
