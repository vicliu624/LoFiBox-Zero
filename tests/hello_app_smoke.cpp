// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstddef>
#include <iostream>
#include <unordered_set>

#include "app/lofibox_app.h"
#include "app/input_event.h"
#include "core/color.h"
#include "core/display_profile.h"

int main()
{
    lofibox::app::LoFiBoxApp app{};
    lofibox::core::Canvas canvas{lofibox::core::kDisplayWidth, lofibox::core::kDisplayHeight};

    app.render(canvas);
    const auto boot_pixels = canvas.pixels();

    app.update();
    app.render(canvas);
    const auto first_loading_pixels = canvas.pixels();

    app.update();
    const auto root_snapshot = app.snapshot();
    if (root_snapshot.current_page != lofibox::app::AppPage::MainMenu) {
        std::cerr << "Expected app to transition from Boot to Main Menu.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    const auto library_snapshot = app.snapshot();
    if (library_snapshot.current_page != lofibox::app::AppPage::MusicIndex) {
        std::cerr << "Expected default Main Menu confirm to enter Music Index.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Backspace, "DEL", '\0'});
    const auto back_snapshot = app.snapshot();
    if (back_snapshot.current_page != lofibox::app::AppPage::MainMenu) {
        std::cerr << "Expected Back to return to Main Menu.\n";
        return 1;
    }

    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Home, "HOME", '\0'});
    app.handleInput(lofibox::app::InputEvent{lofibox::app::InputKey::Enter, "OK", '\0'});
    const auto left_from_home_snapshot = app.snapshot();
    if (left_from_home_snapshot.current_page != lofibox::app::AppPage::Songs) {
        std::cerr << "Expected HOME on Main Menu to move left to Music before confirm.\n";
        return 1;
    }

    app.render(canvas);

    const auto background = canvas.pixel(0, 0);
    std::size_t changed_pixels = 0;
    std::size_t frame_delta = 0;
    std::unordered_set<std::uint32_t> palette{};

    for (std::size_t index = 0; index < canvas.pixels().size(); ++index) {
        const auto& pixel = canvas.pixels()[index];
        if (pixel != background) {
            ++changed_pixels;
        }
        palette.insert(lofibox::core::pack_key(pixel));
        if (pixel != boot_pixels[index]) {
            ++frame_delta;
        }
    }

    if (changed_pixels < 1000) {
        std::cerr << "Expected the app to render more than a trivial number of pixels.\n";
        return 1;
    }

    if (palette.size() < 4) {
        std::cerr << "Expected multiple colors in the rendered app screen.\n";
        return 1;
    }

    if (first_loading_pixels == boot_pixels) {
        std::cerr << "Expected Boot rendering to change once loading begins.\n";
        return 1;
    }

    if (frame_delta == 0) {
        std::cerr << "Expected rendered frames to differ across app states.\n";
        return 1;
    }

    return 0;
}
