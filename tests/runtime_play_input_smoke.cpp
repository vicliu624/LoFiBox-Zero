// SPDX-License-Identifier: GPL-3.0-or-later

#include <cassert>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <unistd.h>

#include "application/app_service_host.h"
#include "cli/runtime_cli.h"
#include "runtime/runtime_host.h"

namespace {
class ScanMetadata final : public lofibox::app::MetadataProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "SCAN-TEST"; }
    lofibox::app::TrackMetadata read(const std::filesystem::path& path, lofibox::app::MetadataReadMode) const override
    {
        entered = true;
        while (blocked) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        lofibox::app::TrackMetadata result;
        result.title = "tag-" + path.stem().string();
        result.duration_seconds = 212;
        return result;
    }
    mutable std::atomic_bool entered{false};
    std::atomic_bool blocked{false};
};
class Backend final : public lofibox::app::AudioPlaybackBackend {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "INPUT-TEST"; }
    bool playFile(const std::filesystem::path& path, double) override
    { file = path; uri.clear(); playing = true; ++starts; return true; }
    bool playUri(const std::string& value, double) override
    { uri = value; file.clear(); playing = true; ++starts; return true; }
    void stop() override { playing = false; }
    bool isPlaying() override { return playing; }
    bool isFinished() override { return false; }
    std::filesystem::path file{};
    std::string uri{};
    bool playing{false};
    int starts{0};
};
}

int main()
{
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / ("lofibox-play-input-" + std::to_string(::getpid()));
    fs::create_directories(root / "outside");
    fs::create_directories(root / "library");
    std::ofstream{root / "library" / "indexed.mp3"} << "library fixture";
    const auto first = root / "outside" / "\xE4\xB8\xAD\xE6\x96\x87 space \"one\".mp3";
    const auto second = root / "outside" / "-second.MP3";
    std::ofstream{first} << "fake audio for recording backend";
    std::ofstream{second} << "fake audio for recording backend";
    auto services = lofibox::app::withNullRuntimeServices();
    auto backend = std::make_shared<Backend>();
    services.playback.audio_backend = backend;
    auto initial_metadata = std::make_shared<ScanMetadata>();
    initial_metadata->blocked = true;
    services.metadata.metadata_provider = initial_metadata;
    lofibox::application::AppServiceHost app{services};
    lofibox::runtime::RuntimeHost host{app.registry()};
    assert(host.startExternalTransport(root / "runtime.sock"));
    auto run = [&](std::vector<std::string> command, int expected = 0) {
        std::vector<std::string> args{"lofibox", "--runtime-socket", (root / "runtime.sock").string(), "--json"};
        args.insert(args.end(), command.begin(), command.end());
        std::vector<char*> argv;
        for (auto& arg : args) argv.push_back(arg.data());
        std::ostringstream out, err;
        const auto rc = lofibox::cli::runRuntimeCliCommand(static_cast<int>(argv.size()), argv.data(), out, err);
        if (!rc || *rc != expected) {
            std::cerr << "unexpected input command result: " << out.str() << err.str() << '\n';
            std::abort();
        }
        return out.str() + err.str();
    };
    const auto started = std::chrono::steady_clock::now();
    run({"play", first.string()});
    assert(std::chrono::steady_clock::now() - started < std::chrono::milliseconds(300));
    assert(backend->file == fs::canonical(first));
    assert(app.controllers().library.model().tracks.size() == 1);
    const auto first_id = app.controllers().library.model().tracks.front().id;
    initial_metadata->blocked = false;
    const auto tag_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (app.controllers().library.findTrack(first_id)->pending_local_title && std::chrono::steady_clock::now() < tag_deadline) {
        host.tick(0.01);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(app.controllers().library.findTrack(first_id)->title == "tag-" + first.stem().string());
    run({"play", first.string()});
    assert(app.controllers().library.model().tracks.size() == 1);
    assert(app.controllers().library.model().tracks.front().id == first_id);

    app.controllers().library.setSongsContextTrackIds("Other album", {first_id});

    const auto cwd = fs::current_path();
    fs::current_path(second.parent_path());
    run({"play", "--", "-second.MP3"});
    fs::current_path(cwd);
    assert(backend->file == fs::canonical(second));
    run({"play", "file://localhost" + (root / "outside").string() + "/%E4%B8%AD%E6%96%87%20space%20%22one%22.mp3"});
    assert(backend->file == fs::canonical(first));

    // A background scan that began before the file was opened can reuse the
    // same numeric ID for another path. The explicit input must remain stable.
    auto metadata = std::make_shared<ScanMetadata>();
    {
        lofibox::app::LibraryController library;
        metadata->blocked = true;
        library.beginAsyncRefreshLibrary({root / "library"}, metadata);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!metadata->entered && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
        assert(metadata->entered);
        const int opened_id = library.registerLocalFile(first);
        assert(opened_id > 0);
        library.findMutableTrack(opened_id)->play_count = 7;
        metadata->blocked = false;
        while (!library.pollAsyncRefreshLibrary() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
        assert(!library.asyncRefreshRunning());
        assert(library.model().tracks.size() == 2);
        assert(library.findTrack(opened_id)->path == fs::canonical(first));
        assert(library.findTrack(opened_id)->play_count == 7);
        assert(library.model().tracks[0].id != library.model().tracks[1].id);
        library.refreshLibrary({root / "library"}, *metadata);
        assert(library.model().tracks.size() == 2);
        const auto removed = root / "removed.mp3";
        std::ofstream{removed} << "temporary file";
        const int removed_id = library.registerLocalFile(removed);
        assert(removed_id > 0);
        fs::remove(removed);
        library.refreshLibrary({root / "library"}, *metadata);
        assert(library.findTrack(removed_id) == nullptr);
        assert(library.model().tracks.size() == 2);
        assert(library.findTrack(opened_id)->title == "tag-" + first.stem().string());
        assert(library.findTrack(opened_id)->duration_seconds == 212);
        assert(library.findTrack(opened_id)->play_count == 7);
        // A supplemental path inside a configured root must not be duplicated.
        const auto indexed = fs::canonical(root / "library" / "indexed.mp3");
        assert(library.registerLocalFile(indexed) > 0);
        library.refreshLibrary({root / "library"}, *metadata);
        assert(library.model().tracks.size() == 2);
    }

    const auto starts = backend->starts;
    for (const auto& args : std::vector<std::vector<std::string>>{
             {"play", (root / "missing.mp3").string()}, {"play", root.string()},
             {"play", "file://other-host/music.mp3"}, {"play", "file:///tmp/bad%00.mp3"},
             {"play", "file:///tmp/bad%QQ.mp3"}, {"play", "nonsense://host/song.mp3"},
             {"play", "https:///missing-host.mp3"}, {"play", first.string(), second.string()},
             {"play", first.string(), "--id", std::to_string(first_id)},
             {"play", "--pause", "--next"}, {"play", "--repeat", "nonsense"},
             {"play", "--source"}, {"play", first.string(), "--item", "other"},
             {"play", "--id", "0"}, {"play", "--typo"}}) {
        run(args, 2);
        assert(backend->starts == starts && backend->file == fs::canonical(first));
    }
    run({"play", "https://user:secret@example.test/music.mp3?token=private"});
    assert(backend->uri == "https://user:secret@example.test/music.mp3?token=private");
    const auto snapshot = run({"now"});
    assert(snapshot.find("music.mp3") != std::string::npos);
    assert(snapshot.find("secret") == std::string::npos && snapshot.find("private") == std::string::npos);
    host.stopExternalTransport();
    fs::remove_all(root);
    std::cout << "runtime file/URI input, selector rejection and secret redaction passed\n";
}
