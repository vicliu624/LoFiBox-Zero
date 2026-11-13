// SPDX-License-Identifier: GPL-3.0-or-later
#include "playback/playback_enrichment_coordinator.h"
#include "core/operation_cancellation.h"
#include "platform/host/runtime_host_internal.h"
#include <cassert>
#include <atomic>
#include <chrono>
#include <thread>
#include <unistd.h>
namespace {
using namespace std::chrono_literals;
struct Connectivity : lofibox::app::ConnectivityProvider {
    std::atomic_bool online{true};
    bool connected() const override { return online; }
    std::string displayName() const override { return "TEST"; }
};
struct Metadata : lofibox::app::MetadataProvider {
    mutable std::atomic<int> started{}, cancelled{}, local{};
    std::function<bool()> unavailable;
    bool available() const override { return true; }
    std::string displayName() const override { return "TEST"; }
    lofibox::app::TrackMetadata read(const std::filesystem::path&, lofibox::app::MetadataReadMode mode) const override {
        if (mode == lofibox::app::MetadataReadMode::LocalOnly) { ++local; return {}; }
        const lofibox::core::OperationCancellationGuard online_guard{unavailable};
        if (!lofibox::core::operationCancelled()) {
            ++started;
            (void)lofibox::platform::host::runtime_detail::captureProcessOutput("/bin/sh", {"-c", "sleep 10"});
            if (lofibox::core::operationCancelled()) ++cancelled;
        }
        return {};
    }
};
template<class P> void waitFor(P p) {
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!p()) { assert(std::chrono::steady_clock::now() < deadline); std::this_thread::sleep_for(5ms); }
}
}
int main() {
    using namespace lofibox;
    auto services = app::withNullRuntimeServices();
    auto network = std::make_shared<Connectivity>(); auto metadata = std::make_shared<Metadata>();
    metadata->unavailable = [network, preferences=services.ui.preferences] {
        return !network->online || preferences->effective(app::RuntimeFeature::BatterySaver);
    };
    services.connectivity.provider = network; services.metadata.metadata_provider = metadata;
    app::LibraryController library; app::TrackRecord track{42, "offline.mp3", "Local title", "Local artist", "Local album"};
    library.mutableModel().tracks.push_back(track);
    app::PlaybackSession session; session.current_track_id = 42; session.status = app::PlaybackStatus::Playing;
    const auto before = std::distance(std::filesystem::directory_iterator{"/proc/self/fd"}, std::filesystem::directory_iterator{});
    {
        app::PlaybackEnrichmentCoordinator worker; worker.setServices(services); worker.request(track);
        waitFor([&] { return metadata->started == 1; }); network->online = false;
        waitFor([&] { worker.applyPending(library, session); return metadata->cancelled == 1; });
        for (int i=0; i<50; ++i) worker.applyPending(library, session);
        assert(metadata->started == 1 && library.model().tracks.front().title == "Local title");
        network->online = true; worker.applyPending(library, session); waitFor([&] { return metadata->started == 2; });
        services.ui.preferences->toggle(app::RuntimeFeature::BatterySaver); waitFor([&] { return metadata->cancelled == 2; });
        worker.request(track); waitFor([&] { return metadata->local >= 3; }); assert(metadata->started == 2);
    }
    assert(std::distance(std::filesystem::directory_iterator{"/proc/self/fd"}, std::filesystem::directory_iterator{}) == before);
}
