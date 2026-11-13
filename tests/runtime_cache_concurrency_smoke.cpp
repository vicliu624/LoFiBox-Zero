// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_host_internal.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <future>
#include <string>
#include <thread>
#include <vector>

using namespace lofibox::platform::host::runtime_detail;

int main()
{
    SharedRuntimeCache cache;
    cache.root = std::filesystem::temp_directory_path()
        / ("lofibox-cache-concurrency-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(cache.root / "metadata");

    // An outer metadata lookup must preserve a nested identity lookup's result,
    // even if both started with the same old snapshot.
    {
        CacheEntrySession metadata{cache, "nested"};
        metadata.entry.metadata.title = "Title";
        {
            CacheEntrySession identity{cache, "nested"};
            identity.entry.identity.found = true;
            identity.entry.identity.recording_mbid = "recording";
            identity.entry.release_mbid = "release";
            identity.entry.track_identity_attempted = true;
            identity.persist();
        }
        metadata.entry.online_metadata_attempted = true;
        metadata.persist();
        assert(metadata.entry.identity.found);
        assert(metadata.entry.identity.recording_mbid == "recording");
        assert(metadata.entry.release_mbid == "release");
    }
    const auto disk = loadMetadataCache(cache, "nested");
    assert(disk.metadata.title == "Title");
    assert(disk.identity.found && disk.track_identity_attempted && disk.online_metadata_attempted);

    // A provider can remain in slow external work while another request reads
    // the same track. Waiting for the gate stands in for the network/decoder.
    std::promise<void> started;
    std::promise<void> release;
    auto gate = release.get_future();
    std::thread slow([&] {
        CacheEntrySession session{cache, "nested"};
        started.set_value();
        gate.wait();
        session.entry.online_artwork_attempted = true;
    });
    started.get_future().wait();
    auto fast = std::async(std::launch::async, [&] {
        CacheEntrySession session{cache, "nested"};
        return session.entry.metadata.title;
    });
    const auto status = fast.wait_for(std::chrono::milliseconds{500});
    release.set_value();
    slow.join();
    assert(status == std::future_status::ready);
    assert(fast.get() == "Title");

    // Force map growth, overlapping snapshots and serialized disk writes from
    // independent fields. Old snapshots must not clear another writer's flags.
    std::atomic<bool> go{false};
    std::vector<std::thread> workers;
    for (int worker = 0; worker < 8; ++worker) {
        workers.emplace_back([&, worker] {
            while (!go.load()) std::this_thread::yield();
            for (int iteration = 0; iteration < 80; ++iteration) {
                CacheEntrySession other{cache, std::to_string(worker) + "-" + std::to_string(iteration)};
                other.entry.metadata.title = "other";
                CacheEntrySession session{cache, "shared"};
                switch (worker) {
                case 0: session.entry.metadata.title = "title"; break;
                case 1: session.entry.metadata.artist = "artist"; break;
                case 2: session.entry.lyrics.synced = "[00:00.00]words"; break;
                case 3: session.entry.identity.recording_mbid = "id"; break;
                case 4: session.entry.online_metadata_attempted = true; break;
                case 5: session.entry.online_artwork_attempted = true; break;
                case 6: session.entry.online_lyrics_attempted = true; break;
                case 7: session.entry.track_identity_attempted = true; break;
                }
                session.persist();
                const auto persisted = loadMetadataCache(cache, "shared");
                assert(!persisted.metadata.title || *persisted.metadata.title == "title");
            }
        });
    }
    go = true;
    for (auto& worker : workers) worker.join();
    {
        CacheEntrySession session{cache, "shared"};
        assert(session.entry.metadata.title == "title");
        assert(session.entry.metadata.artist == "artist");
        assert(session.entry.lyrics.synced == "[00:00.00]words");
        assert(session.entry.identity.recording_mbid == "id");
        assert(session.entry.online_metadata_attempted && session.entry.online_artwork_attempted);
        assert(session.entry.online_lyrics_attempted && session.entry.track_identity_attempted);
    }
    const auto persisted = loadMetadataCache(cache, "shared");
    assert(persisted.metadata.title == "title" && persisted.metadata.artist == "artist");
    assert(persisted.online_metadata_attempted && persisted.online_artwork_attempted);
    assert(persisted.online_lyrics_attempted && persisted.track_identity_attempted);
    std::filesystem::remove_all(cache.root);
}
