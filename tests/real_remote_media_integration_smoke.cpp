// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "app/library_controller.h"
#include "app/remote_media_contract.h"
#include "app/runtime_services.h"
#include "cache/cache_manager.h"
#include "platform/host/runtime_services_factory.h"
#include "playback/playback_controller.h"

namespace {

std::string envValue(const char* name)
{
    if (const char* value = std::getenv(name); value != nullptr) {
        return value;
    }
    return {};
}

struct RealRemoteCase {
    lofibox::app::RemoteServerKind kind{lofibox::app::RemoteServerKind::Jellyfin};
    std::string id{};
    std::string label{};
    std::string url_env{};
    std::string username_env{};
    std::string password_env{};
    std::string expected_stream_fragment{};
};

class UriAcceptingAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "REAL-REMOTE-GOVERNANCE-TEST"; }
    bool playFile(const std::filesystem::path&, double) override { return false; }
    bool playUri(const std::string& uri, double) override
    {
        played_uri = uri;
        stopped = false;
        return true;
    }
    void stop() override { stopped = true; }
    [[nodiscard]] bool isPlaying() override { return !played_uri.empty() && !stopped; }
    [[nodiscard]] bool isFinished() override { return false; }

    std::string played_uri{};
    bool stopped{false};
};

bool configured(const RealRemoteCase& test)
{
    return !envValue(test.url_env.c_str()).empty()
        && !envValue(test.username_env.c_str()).empty()
        && !envValue(test.password_env.c_str()).empty();
}

std::vector<lofibox::app::RemoteTrack> playableTracksFromBrowse(
    const RealRemoteCase& test,
    const lofibox::app::RuntimeServices& services,
    const lofibox::app::RemoteServerProfile& profile,
    const lofibox::app::RemoteSourceSession& session,
    const std::vector<lofibox::app::RemoteCatalogNode>& root_nodes)
{
    std::vector<lofibox::app::RemoteTrack> tracks{};
    std::vector<std::pair<lofibox::app::RemoteCatalogNode, int>> pending{};
    for (const auto& node : root_nodes) {
        pending.push_back({node, 0});
    }

    for (std::size_t cursor = 0; cursor < pending.size() && tracks.empty(); ++cursor) {
        const auto [node, depth] = pending[cursor];
        if (depth > 5 || !node.browsable) {
            continue;
        }
        const auto children = services.remote.remote_catalog_provider->browse(profile, session, node, 50);
        for (const auto& child : children) {
            if (child.playable) {
                tracks.push_back(lofibox::app::RemoteTrack{
                    child.id,
                    child.title,
                    child.artist,
                    child.album,
                    child.album_id,
                    child.duration_seconds,
                    child.source_id,
                    child.source_label,
                    child.artwork_key,
                    child.artwork_url,
                    child.lyrics_plain,
                    child.lyrics_synced,
                    child.lyrics_source,
                    child.fingerprint});
            } else if (child.browsable) {
                pending.push_back({child, depth + 1});
            }
        }
    }
    if (tracks.empty()) {
        std::cout << test.label << " browse is authenticated but exposes no playable audio items.\n";
    }
    return tracks;
}

bool exercisePlaybackGovernance(
    const RealRemoteCase& test,
    lofibox::app::RuntimeServices services,
    const lofibox::app::RemoteServerProfile& profile,
    const lofibox::app::RemoteTrack& track,
    const lofibox::app::ResolvedRemoteStream& stream)
{
    services.playback.audio_backend = std::make_shared<UriAcceptingAudioBackend>();
    services.metadata.lyrics_provider.reset();
    services.metadata.artwork_provider.reset();

    lofibox::app::LibraryController library{};
    library.mergeRemoteTracks(profile, {track});
    const auto ids = library.allSongIdsSorted();
    auto* record = ids.empty() ? nullptr : library.findMutableTrack(ids.front());
    if (record == nullptr) {
        std::cerr << "Expected " << test.label << " remote track to enter the unified Library for playback governance.\n";
        return false;
    }

    lofibox::app::PlaybackController playback{};
    playback.setServices(services);
    if (!playback.startRemoteLibraryTrack(stream, *record, profile, track, test.label, true)) {
        std::cerr << "Expected " << test.label << " remote Library playback to start through the shared playback path.\n";
        return false;
    }

    for (int attempt = 0; attempt < 240; ++attempt) {
        playback.update(0.1, library);
        const auto* updated = library.findTrack(ids.front());
        if (updated != nullptr && !updated->fingerprint.empty()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    const auto* governed = library.findTrack(ids.front());
    if (governed == nullptr || governed->fingerprint.empty()) {
        std::cerr << "Expected " << test.label << " remote playback to persist a local audio fingerprint.\n";
        return false;
    }

    const auto cached = services.cache.cache_manager->getText(
        lofibox::cache::CacheBucket::Metadata,
        lofibox::app::remoteMediaCacheKey(profile, track.id));
    if (!cached || cached->find("fingerprint=") == std::string::npos || cached->find(governed->fingerprint) == std::string::npos) {
        std::cerr << "Expected " << test.label << " governed remote fingerprint to be stored in the metadata cache.\n";
        return false;
    }

    return true;
}

int runCase(const RealRemoteCase& test, const lofibox::app::RuntimeServices& services)
{
    if (!configured(test)) {
        std::cout << test.label << " integration environment is not configured; skipping.\n";
        return 0;
    }

    const auto profile = lofibox::app::RemoteServerProfile{
        test.kind,
        test.id,
        test.label,
        envValue(test.url_env.c_str()),
        envValue(test.username_env.c_str()),
        envValue(test.password_env.c_str()),
        ""};

    const auto session = services.remote.remote_source_provider->probe(profile);
    if (!session.available || session.access_token.empty()) {
        std::cerr << "Expected " << test.label << " probe to return an authenticated session.\n";
        return 1;
    }

    const auto root_nodes = services.remote.remote_catalog_provider->browse(profile, session, {}, 25);
    if (root_nodes.empty()) {
        std::cerr << "Expected " << test.label << " root browse to return catalog nodes.\n";
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
        tracks = playableTracksFromBrowse(test, services, profile, session, root_nodes);
    }
    if (tracks.empty()) {
        return 0;
    }

    const auto& first = tracks.front();
    if (first.title.empty() || first.source_label.empty() || first.source_id.empty()) {
        std::cerr << "Expected " << test.label << " track to be normalized for app consumption.\n";
        return 1;
    }

    const auto stream = services.remote.remote_stream_resolver->resolveTrack(profile, session, first);
    if (!stream || stream->url.empty()) {
        std::cerr << "Expected " << test.label << " resolver to produce a playable stream URL.\n";
        return 1;
    }
    if (stream->url.find(test.expected_stream_fragment) == std::string::npos) {
        std::cerr << "Expected " << test.label << " stream URL to use the provider audio endpoint.\n";
        return 1;
    }
    if (!exercisePlaybackGovernance(test, services, profile, first, *stream)) {
        return 1;
    }

    std::cout << test.label << " integration smoke passed with "
              << tracks.size()
              << " catalog track(s).\n";
    return 0;
}

} // namespace

int main()
{
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

    const std::vector<RealRemoteCase> cases{
        {lofibox::app::RemoteServerKind::Emby,
         "emby-lan",
         "Emby",
         "LOFIBOX_TEST_EMBY_URL",
         "LOFIBOX_TEST_EMBY_USERNAME",
         "LOFIBOX_TEST_EMBY_PASSWORD",
         "/Audio/"},
        {lofibox::app::RemoteServerKind::Jellyfin,
         "jellyfin-lan",
         "Jellyfin",
         "LOFIBOX_TEST_JELLYFIN_URL",
         "LOFIBOX_TEST_JELLYFIN_USERNAME",
         "LOFIBOX_TEST_JELLYFIN_PASSWORD",
         "/Audio/"},
    };

    for (const auto& test : cases) {
        if (const int result = runCase(test, services); result != 0) {
            return result;
        }
    }
    return 0;
}
