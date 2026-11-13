// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/app_runtime_context.h"
#include "app/library_scanner.h"
#include "app/app_projection_builder.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {
using namespace lofibox;
class QueueTestAudio final : public app::AudioPlaybackBackend {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "QUEUE-TEST"; }
    bool playFile(const std::filesystem::path& path, double) override { last = path; active = true; return true; }
    bool playUri(const std::string&, double) override { active = true; return true; }
    void stop() override { active = false; }
    void pause() override { active = false; }
    void resume() override { active = true; }
    bool isPlaying() override { return active; }
    bool isFinished() override { return false; }
    std::filesystem::path last;
    bool active{false};
};
struct QueueFixture {
    app::RuntimeServices services = app::withNullRuntimeServices();
    std::shared_ptr<QueueTestAudio> audio = std::make_shared<QueueTestAudio>();
    std::unique_ptr<application::AppServiceHost> host;
    std::unique_ptr<runtime::RuntimeHost> runtime;
    std::unique_ptr<app::AppRuntimeContext> gui;
    explicit QueueFixture(int count = 3)
    {
        services.playback.audio_backend = audio;
        host = std::make_unique<application::AppServiceHost>(services);
        auto& model = host->controllers().library.mutableModel();
        for (int id = 1; id <= count; ++id) {
            app::TrackRecord track;
            track.id = id;
            track.path = "track-" + std::to_string(id) + ".mp3";
            track.title = count == 3 ? std::vector<std::string>{"Alpha", "Bravo", "Charlie"}[id - 1] : "Track " + std::to_string(id);
            track.artist = "Artist " + std::to_string(id);
            track.added_time = id * 100;
            track.play_count = id * 10;
            track.last_played = id * 1000;
            model.tracks.push_back(std::move(track));
        }
        app::rebuildLibraryIndexes(model);
        host->controllers().library.setSongsContextAll();
        runtime = std::make_unique<runtime::RuntimeHost>(host->registry());
        gui = std::make_unique<app::AppRuntimeContext>(std::vector<std::filesystem::path>{}, ui::UiAssets{}, ui::defaultTheme(), *host, runtime->client());
        gui->showMainMenuPage();
    }
    runtime::RuntimeCommandResult dispatch(runtime::RuntimeCommandKind kind, runtime::RuntimeCommandPayload payload = {})
    {
        return runtime->client().dispatch({kind, std::move(payload), runtime::CommandOrigin::DirectTest});
    }
    void digit(char value) { gui->handleInput(app::InputEvent{app::InputKey::Character, std::string(1, value), value}); }
};
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void playlistStartsItsSelectedTrack()
{
    QueueFixture f;
    f.host->controllers().library.setSongsContextTrackIds("PREVIOUS ALBUM", {1});
    f.gui->pushPage(app::AppPage::Playlists);
    f.gui->moveSelection(2); // Most Played: Charlie, Bravo, Alpha.
    f.digit('6');
    require(f.gui->currentPage() == app::AppPage::PlaylistDetail, "playlist did not open");
    require(f.gui->pageModel().rows.front().first == "Charlie", "smart playlist order incorrect");
    f.digit('6');
    require(f.gui->playbackSession().current_track_id == 3, "playlist selected Charlie but played the previous album's Alpha");
    require(f.host->controllers().playback.queue().active_ids == std::vector<int>({3,2,1}), "playlist queue lost its displayed order");
    f.gui->stepTrack(1);
    require(f.gui->playbackSession().current_track_id == 2, "next track left playlist order");
}
void explicitTrackOutsideBrowseContext()
{
    QueueFixture f;
    f.host->controllers().library.setSongsContextTrackIds("PREVIOUS ALBUM", {1});
    require(f.dispatch(runtime::RuntimeCommandKind::PlaybackStartTrack, runtime::RuntimeCommandPayload::startTrack(3)).applied, "valid explicit start failed");
    require(f.gui->playbackSession().current_track_id == 3 && f.audio->last == "track-3.mp3", "explicit track ID started a different file");
}
void playlistSwitchWhileCurrentIsPlaying()
{
    QueueFixture f;
    require(f.gui->startLibraryTrack(3), "active playlist setup failed");
    f.host->controllers().playback.mutableSession().elapsed_seconds = 12;
    f.gui->pushPage(app::AppPage::Playlists); f.gui->moveSelection(2); f.digit('6'); f.digit('6');
    require(f.host->controllers().playback.queue().active_ids == std::vector<int>({3,2,1}), "already playing selection kept the previous queue origin");
    require(f.gui->playbackSession().elapsed_seconds == 12, "rebasing current playlist restarted the audio");
    f.gui->stepTrack(1); require(f.gui->playbackSession().current_track_id == 2, "rebased playlist next is incorrect");
}
void queueOpensAtActiveItem()
{
    QueueFixture f{20}; require(f.gui->startLibraryTrack(17), "active focus setup failed");
    const int active = f.host->controllers().playback.queue().active_index;
    for (bool through_shortcut : {false,true}) {
        f.gui->showMainMenuPage();
        if (through_shortcut) f.gui->openQueuePage(); else f.gui->pushPage(app::AppPage::Queue);
        const auto selection = f.gui->listSelection();
        require(selection.selected == active && active >= selection.scroll && active < selection.scroll + 5, "queue did not open with active item visible");
    }
}
void queuePageUsesSharedItemsAndJump()
{
    QueueFixture f;
    require(f.gui->startLibraryTrack(1), "queue setup failed");
    f.gui->openQueuePage();
    const auto rows = f.gui->pageModel().rows;
    require(rows.size() == 3 && rows[1].first == "Bravo", "queue page omits actual queue items");
    f.gui->moveSelection(1);
    f.digit('6');
    require(f.gui->playbackSession().current_track_id == 2, "queue selection did not jump to its actual index");
    require(f.host->controllers().playback.queue().active_ids == std::vector<int>({1,2,3}), "queue jump replaced queue order");
    require(f.dispatch(runtime::RuntimeCommandKind::QueueClear).applied, "queue clear failed");
    f.gui->openQueuePage();
    const auto before = f.gui->playbackSession().current_track_id;
    f.digit('6');
    require(f.host->controllers().playback.queue().active_ids.empty() && f.gui->playbackSession().current_track_id == before, "empty queue confirmation restarted another track");
    require(f.runtime->client().query({runtime::RuntimeQueryKind::QueueSnapshot}).queue.active_index == -1, "empty queue has a ghost active index");
    f.digit('8');
    const auto empty = f.runtime->client().query({runtime::RuntimeQueryKind::QueueSnapshot}).queue;
    require(empty.active_ids.empty() && empty.active_index == -1 && empty.selected_index == -1, "mode change created an active index in an empty queue");
}
void missingQueueItemKeepsIndex()
{
    QueueFixture f;
    require(f.gui->startLibraryTrack(1), "missing item setup failed");
    auto& tracks = f.host->controllers().library.mutableModel().tracks;
    std::erase_if(tracks, [](const auto& track) { return track.id == 2; });
    const auto queue = f.runtime->client().query({runtime::RuntimeQueryKind::QueueSnapshot}).queue;
    require(queue.visible_items.size() == 3 && !queue.visible_items[1].playable && queue.visible_items[2].queue_index == 2, "missing queue item shifted row/index mapping");
    f.gui->openQueuePage(); f.gui->moveSelection(1); f.digit('6');
    require(f.gui->playbackSession().current_track_id == 1, "unavailable row played another item");
}
void queueWindowNavigation()
{
    QueueFixture f{200}; require(f.gui->startLibraryTrack(1), "window setup failed"); f.gui->openQueuePage();
    const auto ids = f.host->controllers().playback.queue().active_ids;
    auto model = f.gui->pageModel();
    require(model.rowCount() == 200 && model.rows.size() <= 5 && model.row_offset == 0, "large queue materialized offscreen rows");
    f.gui->moveSelection(137); f.gui->moveSelectionPage(1);
    require(f.gui->listSelection().selected == 141, "window navigation used the materialized count");
    f.gui->moveSelection(58); // Last absolute row, not last row of the window.
    const auto window = app::buildListProjection(*f.gui);
    require(window.total_rows == 200 && window.row_offset == 195 && window.rows.size() == 5 && window.selected == 199, "last queue window indices incorrect");
    auto full = window; full.rows.clear(); full.total_rows = -1; full.row_offset = 0;
    for (int index = 0; index < 200; ++index) {
        const auto* track = f.host->controllers().library.findTrack(ids[static_cast<std::size_t>(index)]);
        full.rows.emplace_back(track->title, (index == 0 ? "NOW - " : "") + track->artist);
    }
    core::Canvas actual{320,170}, expected{320,170};
    ui::pages::renderListPage(actual, window, ui::defaultTheme());
    ui::pages::renderListPage(expected, full, ui::defaultTheme());
    require(actual.pixels() == expected.pixels(), "window rendering differs from the complete-list renderer");
    f.digit('6');
    require(f.gui->playbackSession().current_track_id == ids.back()
        && f.host->controllers().playback.queue().active_index == 199, "window confirm lost the absolute queue index");
    f.gui->openQueuePage();
    require(f.dispatch(runtime::RuntimeCommandKind::QueueClear).applied, "external clear failed");
    const auto cleared = app::buildListProjection(*f.gui);
    require(cleared.total_rows == 0 && cleared.rows.empty() && cleared.empty_label == "QUEUE EMPTY", "clear retained a stale row window");
}
void resolvedRemoteSelectionUsesItsQueue()
{
    QueueFixture f; require(f.gui->startLibraryTrack(1), "remote queue setup failed");
    auto* local = f.host->controllers().library.findMutableTrack(3); local->remote = true;
    local->remote_profile_id = "test"; local->remote_track_id = "remote-3";
    (void)f.host->controllers().library.openSelectedListItem(app::AppPage::Playlists,2);
    (void)f.host->controllers().library.openSelectedListItem(app::AppPage::PlaylistDetail,0);
    app::RemoteServerProfile profile{}; profile.id = "test";
    app::RemoteTrack track{}; track.id = "remote-3"; track.title = "Charlie";
    track.artist = "Artist 3"; track.album = "Album";
    app::ResolvedRemoteStream stream{}; stream.url = "https://example.invalid/fake-no-network";
    const auto payload = runtime::RuntimeCommandPayload::remoteResolvedLibraryTrack(3,profile,track,stream,"TEST",false,{});
    require(f.dispatch(runtime::RuntimeCommandKind::RemoteStartActiveStream,payload).applied, "resolved fake remote start failed");
    require(f.host->controllers().playback.queue().active_ids == std::vector<int>({3,2,1})
        && f.host->controllers().playback.queue().active_index == 0, "explicit remote selection kept the previous queue");
    const auto before = f.host->controllers().playback.queue().active_ids;
    require(!f.dispatch(runtime::RuntimeCommandKind::RemoteStartActiveStream,
        runtime::RuntimeCommandPayload::remoteResolvedLibraryTrack(999,profile,track,stream,"TEST",false,{})).applied,
        "invalid resolved remote ID was accepted");
    require(f.host->controllers().playback.queue().active_ids == before, "invalid resolved remote start replaced the queue");
}
void queueBenchmark()
{
    for (int count : {1000, 4000, 10000}) {
        QueueFixture f{count}; require(f.gui->startLibraryTrack(1), "benchmark start failed");
        f.gui->openQueuePage();
        core::Canvas canvas{320,170};
        double query_ms{}, render_ms{};
        for (int repeat = 0; repeat < 3; ++repeat) {
            auto start = std::chrono::steady_clock::now();
            const auto snapshot = f.runtime->client().query({runtime::RuntimeQueryKind::QueueSnapshot});
            query_ms += std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
            require(static_cast<int>(snapshot.queue.visible_items.size()) == count, "benchmark queue size wrong");
            start = std::chrono::steady_clock::now(); f.gui->render(canvas);
            render_ms += std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        }
        std::cout << "{\"tracks\":" << count << ",\"queue_query_mean_ms\":" << query_ms/3
            << ",\"queue_render_mean_ms\":" << render_ms/3 << "}\n";
    }
}
} // namespace
int main(int argc, char** argv)
{
    if (argc == 2 && std::string_view{argv[1]} == "--benchmark") { queueBenchmark(); return 0; }
    int failed{};
    for (const auto& [name, test] : std::vector<std::pair<const char*,std::function<void()>>>{
        {"playlist origin",playlistStartsItsSelectedTrack},{"explicit track",explicitTrackOutsideBrowseContext},
        {"active playlist origin",playlistSwitchWhileCurrentIsPlaying},{"queue focus",queueOpensAtActiveItem},
        {"queue GUI",queuePageUsesSharedItemsAndJump},{"missing queue item",missingQueueItemKeepsIndex},
        {"queue row window",queueWindowNavigation},{"resolved remote queue",resolvedRemoteSelectionUsesItsQueue}}) {
        try { test(); std::cout << name << ": passed\n"; }
        catch (const std::exception& error) { ++failed; std::cerr << name << ": " << error.what() << '\n'; }
    }
    return failed ? 1 : 0;
}
