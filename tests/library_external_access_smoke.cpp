// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/lofibox_app.h"
#include "app/library_scanner.h"
#include "platform/host/library_query_provider_adapter.h"
#include "platform/host/library_enrich_provider_adapter.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <fstream>
#include <thread>

using namespace lofibox;

class ScanMetadata final : public app::MetadataProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "scan fixture"; }
    app::TrackMetadata read(const std::filesystem::path&, app::MetadataReadMode) const override
    {
        return {.title = "scan", .artist = "artist", .album = "scan"};
    }
};

class SlowArtwork final : public app::ArtworkProvider {
public:
    mutable std::promise<void> entered;
    mutable std::atomic<bool> signaled{false};
    std::shared_future<void> release;
    bool available() const override { return true; }
    std::string displayName() const override { return "slow fixture"; }
    std::optional<core::Canvas> read(const std::filesystem::path&, app::ArtworkReadMode) const override
    {
        if (!signaled.exchange(true)) entered.set_value();
        release.wait();
        return std::nullopt;
    }
};

int main()
{
    auto services = app::withNullRuntimeServices();
    services.metadata.metadata_provider = std::make_shared<ScanMetadata>();
    auto slow = std::make_shared<SlowArtwork>();
    std::promise<void> release;
    slow->release = release.get_future().share();
    const auto root = std::filesystem::temp_directory_path() / ("lofibox-external-library-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    for (int id = 1; id <= 40; ++id) std::ofstream(root / (std::to_string(id) + ".mp3")) << "fixture";
    platform::host::LibraryQueryProviderAdapter adapter{slow, root};
    std::optional<application::AppServiceRegistry> registry;
    runtime::RuntimeCommandClient* client = nullptr;
    app::LoFiBoxApp player{{root}, {}, services, {}, false,
        [&](runtime::RuntimeCommandClient& runtime_client, const application::AppServiceRegistry& app_services) {
            client = &runtime_client;
            registry.emplace(app_services);
            adapter.bind(registry->libraryQueries());
        }};
    auto& library = registry->libraryController();
    library.refreshLibrary({root}, *services.metadata.metadata_provider);
    auto replace = [&](int generation) {
        const std::lock_guard lock{registry->stateMutex()};
        app::LibraryModel next;
        for (int id = 1; id <= 100 + generation % 30; ++id) {
            app::TrackRecord track;
            track.id = id;
            track.title = std::to_string(generation);
            track.artist = "artist";
            track.album = std::to_string(generation);
            track.path = root / (std::to_string(id) + ".mp3");
            next.tracks.push_back(std::move(track));
        }
        app::rebuildLibraryIndexes(next);
        library.mutableModel() = std::move(next);
    };
    replace(0);

    std::atomic<bool> stop{false};
    std::atomic<int> reads{0};
    std::vector<std::thread> readers;
    for (int worker = 0; worker < 3; ++worker) {
        readers.emplace_back([&] {
            while (!stop.load()) {
                const auto tracks = adapter.listTracks();
                assert(!tracks.empty());
                for (const auto& track : tracks) assert(track.title == tracks.front().title);
                const auto albums = adapter.listAlbums();
                assert(albums.size() == 1 && albums.front().first_track_id > 0);
                ++reads;
            }
        });
    }
    readers.emplace_back([&] {
        while (!stop.load()) {
            (void)client->snapshot();
            (void)client->dispatch({runtime::RuntimeCommandKind::EqSetBand,
                runtime::RuntimeCommandPayload::eqSetBand(2, 3)});
        }
    });
    core::Canvas canvas{320, 170};
    for (int generation = 1; generation <= 80; ++generation) {
        replace(generation);
        player.update();
        player.handleInput({app::InputKey::Home, "HOME", '\0'});
        player.render(canvas);
        (void)player.snapshot();
    }
    {
        const std::lock_guard lock{registry->stateMutex()};
        library.beginAsyncRefreshLibrary({root}, services.metadata.metadata_provider);
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (library.asyncRefreshRunning() && std::chrono::steady_clock::now() < deadline) {
        player.update();
        std::this_thread::yield();
    }
    assert(!library.asyncRefreshRunning());
    assert(adapter.listTracks().size() == 40);
    stop = true;
    for (auto& reader : readers) reader.join();
    assert(reads > 0);

    // A network/decoder provider must never retain the controller guard.
    std::thread artwork([&] { (void)adapter.getArtworkPng(1); });
    slow->entered.get_future().wait();
    auto fast = std::async(std::launch::async, [&] {
        player.render(canvas);
        return client->dispatch({runtime::RuntimeCommandKind::EqSetBand,
            runtime::RuntimeCommandPayload::eqSetBand(1, 6)}).applied;
    });
    const auto status = fast.wait_for(std::chrono::milliseconds{500});
    release.set_value();
    artwork.join();
    assert(status == std::future_status::ready && fast.get());

    const auto artwork_key = platform::host::runtime_detail::cacheKeyForPath(root / "1.mp3");
    std::filesystem::create_directories(root / "artwork");
    const auto cache_path = root / "artwork" / (artwork_key + ".png");
    std::ofstream(cache_path, std::ios::binary) << "image";
    std::filesystem::resize_file(cache_path, 16 * 1024 * 1024);
    assert(!adapter.getArtworkPng(1));

    assert(platform::host::wikipediaUrlForTitle("AC/DC & friends").ends_with("AC%2FDC_%26_friends"));
    assert(platform::host::parseWikipediaExtract(R"({"extract" : "\u4e2d\u6587 \ud83c\udfb5"})") == "\xE4\xB8\xAD\xE6\x96\x87 \xF0\x9F\x8E\xB5");
    assert(!platform::host::parseWikipediaExtract(R"({"extract":"truncated)"));
    assert(!platform::host::parseWikipediaExtract(R"({"extract":"\ud800"})"));
    std::vector<std::thread> enrich_requests;
    for (int i = 0; i < 4; ++i) enrich_requests.emplace_back([] { platform::host::respectWikipediaRateLimit(); });
    for (auto& request : enrich_requests) request.join();
    std::filesystem::remove_all(root);
}
