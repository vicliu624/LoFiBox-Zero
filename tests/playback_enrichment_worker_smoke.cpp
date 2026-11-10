// SPDX-License-Identifier: GPL-3.0-or-later
#include "playback/playback_enrichment_coordinator.h"
#include "core/operation_cancellation.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {
using namespace std::chrono_literals;
void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

class BusyMetadata final : public lofibox::app::MetadataProvider {
public:
    mutable std::atomic<int> calls{0}, active{0}, maximum{0};
    std::atomic_bool hold{true};
    bool available() const override { return true; }
    std::string displayName() const override { return "TEST"; }
    lofibox::app::TrackMetadata read(const std::filesystem::path& path, lofibox::app::MetadataReadMode) const override
    {
        ++calls;
        const int concurrent = ++active;
        maximum.store(std::max(maximum.load(), concurrent));
        while (hold && !lofibox::core::operationCancelled()) std::this_thread::sleep_for(1ms);
        --active;
        lofibox::app::TrackMetadata result{};
        result.title = path.stem().string();
        return result;
    }
};

template<class Predicate> void waitUntil(Predicate condition)
{
    const auto limit = std::chrono::steady_clock::now() + 2s;
    while (!condition()) {
        expect(std::chrono::steady_clock::now() < limit, "background worker timed out");
        std::this_thread::sleep_for(1ms);
    }
}
} // namespace

int main()
{
    try {
        const auto metadata = std::make_shared<BusyMetadata>();
        auto services = lofibox::app::withNullRuntimeServices();
        services.metadata.metadata_provider = metadata;
        lofibox::app::LibraryController library;
        lofibox::app::TrackRecord latest{200, "latest.flac", "", "Artist", "Album"};
        library.mutableModel().tracks.push_back(latest);
        lofibox::app::PlaybackSession session{};
        session.current_track_id = latest.id;
        session.lyrics_lookup_pending = true;
        auto coordinator = std::make_unique<lofibox::app::PlaybackEnrichmentCoordinator>();
        coordinator->setServices(services);
        coordinator->request(lofibox::app::TrackRecord{1, "first.flac", "first", "Artist", "Album"});
        waitUntil([&] { return metadata->calls.load() > 0; });
        const auto submitting = std::chrono::steady_clock::now();
        for (int id = 2; id < 200; ++id) {
            coordinator->request(lofibox::app::TrackRecord{id, "skipped.flac", "old", "Artist", "Album"});
        }
        coordinator->request(latest);
        expect(std::chrono::steady_clock::now() - submitting < 1s, "submitting tracks waited for background work");
        metadata->hold = false;
        waitUntil([&] { coordinator->applyPending(library, session); return !session.lyrics_lookup_pending; });
        expect(metadata->maximum == 1, "rapid changes started concurrent enrichment jobs");
        expect(library.model().tracks.front().title == "latest", "latest track was not enriched");
        metadata->hold = true;
        const int previous = metadata->calls;
        coordinator->request(latest);
        waitUntil([&] { return metadata->calls.load() > previous; });
        const auto stopping = std::chrono::steady_clock::now();
        coordinator.reset();
        expect(std::chrono::steady_clock::now() - stopping < 1s, "shutdown did not cancel active enrichment");
        expect(metadata->active == 0, "worker survived coordinator destruction");
        std::cout << "Latest-track coalescing, bounded concurrency and shutdown cancellation passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
