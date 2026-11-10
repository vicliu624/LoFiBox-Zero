// SPDX-License-Identifier: GPL-3.0-or-later
#include "playback/playback_controller.h"
#include "core/operation_cancellation.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {
using namespace std::chrono_literals;
using namespace lofibox;

void expect(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

void waitGate(const std::atomic_bool& open)
{
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!open && !core::operationCancelled() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
}

template<class Predicate> void waitUntil(Predicate condition)
{
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!condition()) {
        expect(std::chrono::steady_clock::now() < deadline, "background stage did not settle");
        std::this_thread::sleep_for(1ms);
    }
}

class GatedMetadata final : public app::MetadataProvider {
public:
    std::atomic_bool local_open{false}, online_open{false};
    mutable std::atomic<int> local_calls{0}, online_calls{0};
    bool available() const override { return true; }
    std::string displayName() const override { return "GATED"; }
    app::TrackMetadata read(const std::filesystem::path& path, app::MetadataReadMode mode) const override
    {
        const bool local = mode == app::MetadataReadMode::LocalOnly;
        if (local) { ++local_calls; waitGate(local_open); }
        else { ++online_calls; waitGate(online_open); }
        app::TrackMetadata result{};
        result.title = (local ? "local-" : "online-") + path.stem().string();
        result.duration_seconds = local ? 201 : 202;
        return result;
    }
};

class GatedArtwork final : public app::ArtworkProvider {
public:
    std::atomic_bool open{false};
    mutable std::atomic<int> local_calls{0};
    bool available() const override { return true; }
    std::string displayName() const override { return "GATED"; }
    std::optional<core::Canvas> read(const std::filesystem::path&, app::ArtworkReadMode mode) const override
    {
        if (mode == app::ArtworkReadMode::LocalOnly) { ++local_calls; waitGate(open); }
        core::Canvas result{2, 2};
        result.clear(core::Color{1, 2, 3, 255});
        return result;
    }
};

class TestLyrics final : public app::LyricsProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "TEST"; }
    app::TrackLyrics fetch(const std::filesystem::path&, const app::TrackMetadata& metadata) const override
    {
        app::TrackLyrics result{};
        result.plain = "lyrics-" + metadata.title.value_or("");
        return result;
    }
};

class TestAudio final : public app::AudioPlaybackBackend {
public:
    bool active{false};
    int starts{0};
    bool available() const override { return true; }
    std::string displayName() const override { return "TEST"; }
    bool playFile(const std::filesystem::path&, double) override { active = true; ++starts; return true; }
    void stop() override { active = false; }
    void pause() override { active = false; }
    void resume() override { active = true; }
    bool isPlaying() override { return active; }
    bool isFinished() override { return false; }
};
} // namespace

int main()
{
    try {
        auto services = app::withNullRuntimeServices();
        const auto metadata = std::make_shared<GatedMetadata>();
        const auto artwork = std::make_shared<GatedArtwork>();
        const auto audio = std::make_shared<TestAudio>();
        services.metadata.metadata_provider = metadata;
        services.metadata.artwork_provider = artwork;
        services.metadata.lyrics_provider = std::make_shared<TestLyrics>();
        services.playback.audio_backend = audio;
        app::LibraryController library;
        app::TrackRecord first{1, "first.flac", "Indexed first", "Artist", "Album"};
        first.lyrics_plain = "indexed lyrics";
        first.lyrics_source = "INDEX";
        library.mutableModel().tracks = {first, {2, "second.flac", "Indexed second", "Artist", "Album"}};
        app::PlaybackController playback;
        playback.setServices(services);

        const auto start = std::chrono::steady_clock::now();
        expect(playback.startTrack(library, 1), "start rejected valid track");
        expect(std::chrono::steady_clock::now() - start < 300ms, "play command waited for local enrichment");
        waitUntil([&] { return metadata->local_calls > 0; });
        expect(audio->starts == 1 && playback.session().audio_active, "audio did not start before metadata");
        expect(playback.session().current_lyrics.plain == "indexed lyrics", "indexed lyrics unavailable during local lookup");
        const auto pause = std::chrono::steady_clock::now();
        playback.pause();playback.resume();
        expect(std::chrono::steady_clock::now() - pause < 300ms, "pause/resume waited for local metadata");
        metadata->local_open = true;
        waitUntil([&] { return artwork->local_calls > 0; });
        const auto blocked_cover = std::chrono::steady_clock::now();
        playback.pause();playback.resume();
        expect(std::chrono::steady_clock::now() - blocked_cover < 300ms, "pause/resume waited for artwork");
        artwork->open = true;
        waitUntil([&] { return metadata->online_calls > 0; });
        waitUntil([&] { playback.update(0, library);return playback.session().current_artwork.has_value(); });
        expect(library.findTrack(1)->duration_seconds == 201, "local tags waited for network completion");
        expect(library.findTrack(1)->title == "Indexed first", "local phase replaced an existing indexed title");
        expect(playback.session().lyrics_lookup_pending, "partial local result finished lyrics lookup");
        expect(playback.session().current_lyrics.plain == "indexed lyrics", "partial local result erased indexed lyrics");

        const auto next = std::chrono::steady_clock::now();
        expect(playback.stepQueue(library, 1), "next track failed");
        expect(std::chrono::steady_clock::now() - next < 300ms, "next waited for previous online work");
        waitUntil([&] { playback.update(0, library);return library.findTrack(2)->duration_seconds == 201; });
        expect(playback.session().current_track_id == 2, "stale result changed current track");
        metadata->online_open = true;
        waitUntil([&] { playback.update(0, library);return !playback.session().lyrics_lookup_pending; });
        expect(library.findTrack(2)->duration_seconds == 202, "final metadata did not apply");
        expect(library.findTrack(2)->title == "Indexed second", "online phase replaced an existing indexed title");
        expect(playback.session().current_lyrics.plain == "lyrics-online-second", "stale lyrics replaced latest track");
        expect(library.findTrack(1)->play_count == 1 && library.findTrack(2)->play_count == 1,
               "async enrichment changed play count accounting");
        std::cout << "Start/pause/next remain responsive with blocked local and online providers; local facts publish first.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
