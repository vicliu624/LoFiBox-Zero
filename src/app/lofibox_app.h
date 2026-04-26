// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "app/input_event.h"
#include "app/playback_state.h"
#include "app/runtime_services.h"
#include "core/canvas.h"

namespace lofibox::app {

enum class AppPage {
    Boot,
    MainMenu,
    MusicIndex,
    Artists,
    Albums,
    Songs,
    Genres,
    Composers,
    Compilations,
    Playlists,
    PlaylistDetail,
    NowPlaying,
    Lyrics,
    Equalizer,
    Settings,
    About,
};

struct AppDebugSnapshot {
    AppPage current_page{AppPage::Boot};
    bool library_ready{false};
    std::size_t track_count{0};
    std::size_t visible_count{0};
    int list_selected_index{0};
    int list_scroll_offset{0};
    bool playback_active{false};
    PlaybackStatus playback_status{PlaybackStatus::Empty};
    std::optional<int> current_track_id{};
    bool shuffle_enabled{false};
    bool repeat_all{false};
    bool repeat_one{false};
    bool network_connected{false};
    int eq_band_count{0};
    int eq_selected_band{0};
};

struct AppAssets {
    std::optional<core::Canvas> logo{};
    std::optional<core::Canvas> music_icon{};
    std::optional<core::Canvas> library_icon{};
    std::optional<core::Canvas> playlists_icon{};
    std::optional<core::Canvas> now_playing_icon{};
    std::optional<core::Canvas> equalizer_icon{};
    std::optional<core::Canvas> settings_icon{};
};

class LoFiBoxApp {
public:
    explicit LoFiBoxApp(std::vector<std::filesystem::path> media_roots = {}, AppAssets assets = {}, RuntimeServices services = {});
    ~LoFiBoxApp();

    void update();
    void handleInput(const InputEvent& event);
    void render(core::Canvas& canvas) const;

    [[nodiscard]] AppDebugSnapshot snapshot() const;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_{};
};

} // namespace lofibox::app
