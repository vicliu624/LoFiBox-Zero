// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>

#include "app/library_controller.h"
#include "app/playback_controller.h"
#include "app/runtime_services.h"

namespace fs = std::filesystem;

namespace {

void touchFile(const fs::path& path)
{
    std::ofstream output(path.string(), std::ios::binary);
    output << "test";
}

class FinishingAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "TEST"; }
    bool playFile(const fs::path& path, double) override
    {
        finished = false;
        ++play_count;
        last_path = path;
        return true;
    }
    void stop() override { finished = true; }
    void pause() override { paused = true; }
    void resume() override { paused = false; }
    [[nodiscard]] bool isPlaying() override { return !finished && !paused; }
    [[nodiscard]] bool isFinished() override { return finished; }

    bool finished{false};
    bool paused{false};
    int play_count{0};
    fs::path last_path{};
};

class FinishOnSecondPollBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "POLL-RACE-TEST"; }
    bool playFile(const fs::path& path, double) override
    {
        last_path = path;
        finished_checks = 0;
        ++play_count;
        return true;
    }
    void stop() override {}
    void pause() override { paused = true; }
    void resume() override { paused = false; }
    [[nodiscard]] bool isPlaying() override { return !paused; }
    [[nodiscard]] bool isFinished() override { return ++finished_checks >= 2; }

    bool paused{false};
    int finished_checks{0};
    int play_count{0};
    fs::path last_path{};
};

class FailedAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "FAILED-TEST"; }
    bool playFile(const fs::path& path, double) override
    {
        last_path = path;
        state_value = lofibox::app::AudioPlaybackState::Failed;
        ++play_count;
        return true;
    }
    void stop() override { state_value = lofibox::app::AudioPlaybackState::Idle; }
    [[nodiscard]] bool isPlaying() override { return false; }
    [[nodiscard]] bool isFinished() override { return false; }
    [[nodiscard]] lofibox::app::AudioPlaybackState state() override { return state_value; }

    lofibox::app::AudioPlaybackState state_value{lofibox::app::AudioPlaybackState::Idle};
    int play_count{0};
    fs::path last_path{};
};

} // namespace

int main()
{
    const fs::path root = fs::temp_directory_path() / "lofibox_zero_playback_controller_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root / "Artist" / "Album");
    touchFile(root / "Artist" / "Album" / "alpha.mp3");
    touchFile(root / "Artist" / "Album" / "beta.mp3");
    touchFile(root / "Artist" / "Album" / "gamma.mp3");

    auto services = lofibox::app::withNullRuntimeServices();
    auto backend = std::make_shared<FinishingAudioBackend>();
    services.playback.audio_backend = backend;
    lofibox::app::LibraryController library{};
    library.startLoading();
    library.refreshLibrary({root}, *services.metadata.metadata_provider);
    library.setSongsContextAll();

    const auto ids = library.trackIdsForCurrentSongs();
    if (ids.size() != 3) {
        std::cerr << "Expected three tracks in playback source context.\n";
        return 1;
    }

    lofibox::app::PlaybackController playback{};
    playback.setServices(services);
    if (!playback.startTrack(library, ids.front())) {
        std::cerr << "Expected playback controller to accept a valid track id.\n";
        return 1;
    }

    if (playback.session().status != lofibox::app::PlaybackStatus::Playing || !playback.session().current_track_id) {
        std::cerr << "Expected playback session to enter Playing state with a current track.\n";
        return 1;
    }

    playback.update(2.5, library);
    if (playback.session().elapsed_seconds < 2.0) {
        std::cerr << "Expected playback update to advance elapsed time while playing.\n";
        return 1;
    }

    playback.togglePlayPause();
    if (playback.session().status != lofibox::app::PlaybackStatus::Paused) {
        std::cerr << "Expected toggle to pause playback.\n";
        return 1;
    }
    if (!backend->paused || backend->isPlaying()) {
        std::cerr << "Expected toggle pause to pause the audio backend.\n";
        return 1;
    }
    const double paused_elapsed = playback.session().elapsed_seconds;
    playback.update(3.0, library);
    if (playback.session().elapsed_seconds != paused_elapsed) {
        std::cerr << "Expected paused playback not to advance elapsed time.\n";
        return 1;
    }
    playback.togglePlayPause();
    if (playback.session().status != lofibox::app::PlaybackStatus::Playing) {
        std::cerr << "Expected toggle to resume playback.\n";
        return 1;
    }
    if (backend->paused || !backend->isPlaying()) {
        std::cerr << "Expected toggle resume to resume the audio backend.\n";
        return 1;
    }

    const auto first_track_id = *playback.session().current_track_id;
    playback.stepQueue(library, 1);
    if (!playback.session().current_track_id || *playback.session().current_track_id == first_track_id) {
        std::cerr << "Expected queue step to move to the next track.\n";
        return 1;
    }

    playback.setRepeatOne(true);
    const int repeat_one_track_id = *playback.session().current_track_id;
    playback.stepQueue(library, 1);
    if (!playback.session().current_track_id || *playback.session().current_track_id == repeat_one_track_id) {
        std::cerr << "Expected manual next to work even in repeat-one mode.\n";
        return 1;
    }

    playback.setRepeatOne(false);
    if (!playback.startTrack(library, ids.front())) {
        std::cerr << "Expected playback controller to start a track with a finishing backend.\n";
        return 1;
    }
    playback.update(4.0, library);
    const double before_finish = playback.session().elapsed_seconds;
    backend->finished = true;
    playback.update(4.0, library);
    if (!playback.session().current_track_id || *playback.session().current_track_id == ids.front()) {
        std::cerr << "Expected normal playback finish to advance to next track when one exists.\n";
        return 1;
    }
    if (playback.session().elapsed_seconds > 0.01 || playback.session().status != lofibox::app::PlaybackStatus::Playing) {
        std::cerr << "Expected auto-advanced track to restart elapsed time and keep playing.\n";
        return 1;
    }

    if (!playback.startTrack(library, ids.back())) {
        std::cerr << "Expected last track to start.\n";
        return 1;
    }
    playback.setRepeatAll(false);
    backend->finished = true;
    playback.update(1.0, library);
    if (playback.session().status != lofibox::app::PlaybackStatus::Paused) {
        std::cerr << "Expected normal playback to pause when the last track finishes.\n";
        return 1;
    }
    if (playback.session().elapsed_seconds > before_finish + 10.0) {
        std::cerr << "Expected elapsed time to remain bounded after final finish.\n";
        return 1;
    }

    playback.setRepeatAll(true);
    if (!playback.startTrack(library, ids.back())) {
        std::cerr << "Expected last track to restart for repeat-all test.\n";
        return 1;
    }
    playback.stepQueue(library, 1);
    if (!playback.session().current_track_id || *playback.session().current_track_id != ids.front()) {
        std::cerr << "Expected repeat-all next from last track to wrap to first track.\n";
        return 1;
    }
    playback.stepQueue(library, -1);
    if (!playback.session().current_track_id || *playback.session().current_track_id != ids.back()) {
        std::cerr << "Expected repeat-all previous from first track to wrap to last track.\n";
        return 1;
    }
    backend->finished = true;
    playback.update(1.0, library);
    if (!playback.session().current_track_id || *playback.session().current_track_id != ids.front()) {
        std::cerr << "Expected repeat-all finish on last track to wrap to first track.\n";
        return 1;
    }

    playback.setRepeatOne(true);
    if (!playback.startTrack(library, ids[1])) {
        std::cerr << "Expected middle track to start for repeat-one test.\n";
        return 1;
    }
    backend->finished = true;
    playback.update(1.0, library);
    if (!playback.session().current_track_id || *playback.session().current_track_id != ids[1]) {
        std::cerr << "Expected repeat-one finish to restart the same track.\n";
        return 1;
    }

    playback.setRepeatOne(false);
    playback.setRepeatAll(true);
    if (!playback.startTrack(library, ids.front())) {
        std::cerr << "Expected first track to start for shuffle test.\n";
        return 1;
    }
    playback.setShuffleEnabled(true);
    if (!playback.session().shuffle_enabled) {
        std::cerr << "Expected shuffle mode to be enabled.\n";
        return 1;
    }
    std::set<int> visited{};
    visited.insert(*playback.session().current_track_id);
    for (int index = 0; index < static_cast<int>(ids.size()) * 2; ++index) {
        playback.stepQueue(library, 1);
        if (playback.session().current_track_id) {
            visited.insert(*playback.session().current_track_id);
        }
    }
    if (visited.size() != ids.size()) {
        std::cerr << "Expected shuffle queue navigation to keep every track reachable.\n";
        return 1;
    }

    auto race_services = lofibox::app::withNullRuntimeServices();
    auto race_backend = std::make_shared<FinishOnSecondPollBackend>();
    race_services.playback.audio_backend = race_backend;
    lofibox::app::PlaybackController race_playback{};
    race_playback.setServices(race_services);
    library.setSongsContextAll();
    if (!race_playback.startTrack(library, ids.front())) {
        std::cerr << "Expected race playback controller to start a track.\n";
        return 1;
    }
    race_playback.update(0.016, library);
    if (!race_playback.session().current_track_id || *race_playback.session().current_track_id != ids.front()) {
        std::cerr << "Expected playback finish polling to be consumed at most once per UI update.\n";
        return 1;
    }
    race_playback.update(0.016, library);
    if (!race_playback.session().current_track_id || *race_playback.session().current_track_id == ids.front()) {
        std::cerr << "Expected deferred finish polling to advance on the next update.\n";
        return 1;
    }

    auto failed_services = lofibox::app::withNullRuntimeServices();
    auto failed_backend = std::make_shared<FailedAudioBackend>();
    failed_services.playback.audio_backend = failed_backend;
    lofibox::app::PlaybackController failed_playback{};
    failed_playback.setServices(failed_services);
    if (!failed_playback.startTrack(library, ids.front())) {
        std::cerr << "Expected failed backend test to start a track request.\n";
        return 1;
    }
    failed_playback.update(0.016, library);
    if (!failed_playback.session().current_track_id || *failed_playback.session().current_track_id != ids.front()) {
        std::cerr << "Expected playback backend failure not to auto-advance the queue.\n";
        return 1;
    }
    if (failed_playback.session().status != lofibox::app::PlaybackStatus::Paused) {
        std::cerr << "Expected playback backend failure to pause the current session.\n";
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
