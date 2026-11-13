// SPDX-License-Identifier: GPL-3.0-or-later
#include "playback/playback_enrichment_coordinator.h"
#include "platform/host/host_runtime_service_providers.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <unistd.h>

namespace {
struct Network final : lofibox::app::ConnectivityProvider {
    explicit Network(bool value) : online(value) {}
    bool connected() const override { return online; }
    std::string displayName() const override { return "FIXTURE"; }
    bool online;
};
}
int main()
{
    using namespace lofibox;
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;
    const auto root = fs::temp_directory_path() / ("lofibox-local-lyrics-" + std::to_string(getpid()));
    fs::create_directories(root);
    setenv("XDG_CONFIG_HOME", root.c_str(), 1);
    setenv("XDG_CACHE_HOME", root.c_str(), 1);
    const auto reader = root / "local-lyrics-reader";
    // Exercise the real host provider's local-helper path without requiring
    // Mutagen or a particular codec on the test host. No network is involved.
    std::ofstream{reader} << "#!/bin/sh\nprintf 'synced=[00:00.00]Local offline line\\n'\n";
    fs::permissions(reader, fs::perms::owner_all);
    setenv("PYTHON_EXECUTABLE", reader.c_str(), 1);
    setenv("CURL_PATH", "/bin/false", 1);
    for (const bool battery : {false, true}) {
        auto services = app::withNullRuntimeServices();
        auto context = platform::host::createHostRuntimeServiceContext();
        context.preferences = std::make_shared<app::RuntimePreferences>(
            app::RuntimePreferences::kDefaults | (battery ? 32U : 0U));
        context.connectivity.provider = std::make_shared<Network>(battery);
        services.ui.preferences = context.preferences;
        services.connectivity = context.connectivity;
        services.metadata.lyrics_provider = platform::host::createHostMetadataServices(context).lyrics_provider;
        const auto file = root / (battery ? "battery.mp3" : "offline.mp3");
        std::ofstream{file} << "fixture";
        app::TrackRecord track{42, file, "Fixture", "Artist", "Album"};
        app::LibraryController library; library.mutableModel().tracks.push_back(track);
        app::PlaybackSession session; session.current_track_id = 42;
        session.status = app::PlaybackStatus::Playing; session.lyrics_lookup_pending = true;
        {
            app::PlaybackEnrichmentCoordinator worker; worker.setServices(services); worker.request(track);
            const auto end = std::chrono::steady_clock::now() + 3s;
            while (session.lyrics_lookup_pending && std::chrono::steady_clock::now() < end) {
                worker.applyPending(library, session); std::this_thread::sleep_for(5ms);
            }
            assert(!session.lyrics_lookup_pending);
            assert(session.current_lyrics.synced == "[00:00.00]Local offline line");
        }
    }
    fs::remove_all(root);
}
