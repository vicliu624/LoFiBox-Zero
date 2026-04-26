// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "app/runtime_services.h"
#include "platform/host/runtime_services_factory.h"

namespace {

std::string envValue(const char* name)
{
    if (const char* value = std::getenv(name); value != nullptr) {
        return value;
    }
    return {};
}

bool hasAllRealEmbyEnv()
{
    return !envValue("LOFIBOX_TEST_EMBY_URL").empty()
        && !envValue("LOFIBOX_TEST_EMBY_USERNAME").empty()
        && !envValue("LOFIBOX_TEST_EMBY_PASSWORD").empty();
}

} // namespace

int main()
{
    if (!hasAllRealEmbyEnv()) {
        std::cout << "Real Emby integration environment is not configured; skipping.\n";
        return 0;
    }

    auto services = lofibox::platform::host::createHostRuntimeServices();
    if (!services.remote.remote_source_provider
        || !services.remote.remote_catalog_provider
        || !services.remote.remote_stream_resolver
        || !services.remote.remote_source_provider->available()
        || !services.remote.remote_catalog_provider->available()
        || !services.remote.remote_stream_resolver->available()) {
        std::cerr << "Remote media runtime services are unavailable.\n";
        return 1;
    }

    const auto profile = lofibox::app::RemoteServerProfile{
        lofibox::app::RemoteServerKind::Emby,
        "real-emby",
        "Real Emby",
        envValue("LOFIBOX_TEST_EMBY_URL"),
        envValue("LOFIBOX_TEST_EMBY_USERNAME"),
        envValue("LOFIBOX_TEST_EMBY_PASSWORD"),
        ""};

    const auto session = services.remote.remote_source_provider->probe(profile);
    if (!session.available || session.access_token.empty() || session.user_id.empty()) {
        std::cerr << "Expected real Emby probe to return an authenticated session.\n";
        return 1;
    }

    auto tracks = services.remote.remote_catalog_provider->libraryTracks(profile, session, 25);
    if (tracks.empty()) {
        tracks = services.remote.remote_catalog_provider->recentTracks(profile, session, 25);
    }
    if (tracks.empty()) {
        tracks = services.remote.remote_catalog_provider->searchTracks(profile, session, "a", 25);
    }
    if (tracks.empty()) {
        std::cerr << "Expected real Emby catalog to return at least one audio track.\n";
        return 1;
    }

    const auto stream = services.remote.remote_stream_resolver->resolveTrack(profile, session, tracks.front());
    if (!stream || stream->url.empty()) {
        std::cerr << "Expected real Emby resolver to produce a playable stream URL.\n";
        return 1;
    }

    if (stream->url.find("/Audio/") == std::string::npos) {
        std::cerr << "Expected real Emby stream URL to use the Emby audio endpoint.\n";
        return 1;
    }

    std::cout << "Real Emby integration smoke passed with "
              << tracks.size()
              << " catalog track(s).\n";
    return 0;
}
