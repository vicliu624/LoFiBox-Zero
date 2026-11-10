// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/app_runtime_context.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"
#include "core/display_profile.h"
#include "app/page_screen_keys.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main()
{
    namespace fs = std::filesystem;
    using namespace lofibox;
    const auto root = fs::temp_directory_path() / ("lofibox-page-matrix-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    try {
        auto services = app::withNullRuntimeServices();
        application::AppServiceHost host{services};
        runtime::RuntimeHost runtime{host.registry()};
        app::AppRuntimeContext context{{root}, {}, ui::defaultTheme(), host, runtime.client()};
        core::Canvas canvas{core::kDisplayWidth, core::kDisplayHeight};
        double maximum_us = 0;
        int rendered = 0;
        for (bool populated : {false, true}) {
            if (populated) {
                for (int track = 0; track < 40; ++track) {
                    std::ofstream(root / ("Artist - Long title " + std::to_string(track) + ".mp3")) << "fixture";
                }
            }
            host.controllers().library.refreshLibrary({root}, *services.metadata.metadata_provider);
            for (int page_id = 0; page_id <= static_cast<int>(app::AppPage::About); ++page_id) {
                const auto page = static_cast<app::AppPage>(page_id);
                context.showMainMenuPage();
                context.pushPage(page);
                context.moveSelection(100000);
                context.moveSelection(-100000);
                const auto started = std::chrono::steady_clock::now();
                context.render(canvas);
                maximum_us = std::max(maximum_us,
                    std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - started).count());
                if (context.currentPage() != page || canvas.pixels().size() != 320U * 170U) {
                    throw std::runtime_error("page rendering changed navigation or canvas bounds");
                }
                if (page != app::AppPage::Boot && page != app::AppPage::Search && page != app::AppPage::RemoteFieldEditor) {
                    const auto keys = app::screenKeysForPage(page);
                    for (std::size_t index = 0; index < keys.size(); ++index) {
                        if (keys[index].action == app::ScreenKeyAction::None) continue;
                        bool visible = false;
                        for (int y = 147; y < 166; ++y)
                            for (int x = app::kScreenKeyCenters[index]-9; x < app::kScreenKeyCenters[index]+10; ++x)
                                visible |= canvas.pixel(x, y) != context.theme().palette.background;
                        if (!visible) throw std::runtime_error("screen action icon missing or misaligned");
                    }
                }
                if (const char* capture = std::getenv("LOFIBOX_TEST_PAGE_CAPTURE_DIR")) {
                    fs::create_directories(capture);
                    const auto path = fs::path(capture) / (std::to_string(page_id) + (populated ? "-populated.ppm" : "-empty.ppm"));
                    std::ofstream output(path, std::ios::binary);
                    output << "P6\n320 170\n255\n";
                    for (const auto& pixel : canvas.pixels()) {
                        const char rgb[]{static_cast<char>(pixel.r), static_cast<char>(pixel.g), static_cast<char>(pixel.b)};
                        output.write(rgb, 3);
                    }
                }
                if (page != app::AppPage::Boot) {
                    context.handleInput(app::InputEvent{app::InputKey::F1, "F1", '\0'});
                    if (!context.helpOpen()) throw std::runtime_error("page did not open its F1 help");
                    context.render(canvas);
                    context.handleInput(app::InputEvent{app::InputKey::Enter, "OK", '\0'});
                    if (context.helpOpen() || context.currentPage() != page) {
                        throw std::runtime_error("confirm changed page instead of closing help");
                    }
                    context.render(canvas);
                }
                context.popPage();
                if (context.currentPage() != app::AppPage::MainMenu) throw std::runtime_error("page stack did not return");
                ++rendered;
            }
        }
        context.pushPage(app::AppPage::MusicIndex);
        context.moveSelection(5);
        const auto selection = context.listSelection();
        if (context.pageModel().max_visible_rows != ui::defaultTheme().spacing.max_visible_rows
            || selection.selected != 5 || selection.scroll != 1) {
            throw std::runtime_error("list selection can fall behind the screen action bar");
        }
        context.pushPage(app::AppPage::NowPlaying);
        auto& playback = host.controllers().playback.mutableSession();
        playback.status = app::PlaybackStatus::Playing;
        context.render(canvas);
        const auto playing_pixels = canvas.pixels();
        playback.status = app::PlaybackStatus::Paused;
        context.render(canvas);
        bool icon_changed = false;
        for (int y = 147; y < 166; ++y) {
            for (int x = 77; x < 133; ++x) {
                icon_changed |= canvas.pixel(x, y) != playing_pixels[static_cast<std::size_t>(y * 320 + x)];
            }
        }
        if (!icon_changed) throw std::runtime_error("play/pause icon did not update");
        auto alternate_theme = ui::defaultTheme();
        alternate_theme.palette.progress = core::rgba(230, 90, 70);
        app::AppRuntimeContext themed{{root}, {}, alternate_theme, host, runtime.client()};
        themed.pushPage(app::AppPage::NowPlaying);
        themed.render(canvas);
        if (canvas.pixel(103, 155) != alternate_theme.palette.progress) {
            throw std::runtime_error("action bar retained the previous theme");
        }
        fs::remove_all(root);
        std::cout << "Rendered " << rendered << " page/state combinations with F1 and navigation checks; host max base render "
            << maximum_us << " us.\n";
        return 0;
    } catch (const std::exception& error) {
        fs::remove_all(root);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
