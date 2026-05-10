// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include "app/input_event.h"
#include "groove/groove_controller.h"
#include "ui/pages/groove/pocket_groove_main_view.h"

namespace lofibox::app {

class GroovePlaybackControl {
public:
    virtual ~GroovePlaybackControl() = default;
    virtual void pauseCurrentPlaybackForGroove() = 0;
};

class AppGrooveBridge {
public:
    AppGrooveBridge();
    explicit AppGrooveBridge(lofibox::groove::GrooveProject project);

    [[nodiscard]] bool active() const noexcept;
    void enter(GroovePlaybackControl& playback);
    void exit();

    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> dispatch(const lofibox::groove::PocketGrooveCommand& command);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleInput(const InputEvent& event, bool fn_held = false);

    [[nodiscard]] const lofibox::groove::GrooveController& controller() const noexcept;
    [[nodiscard]] lofibox::ui::pages::groove::PocketGrooveMainView mainView() const;

private:
    lofibox::groove::GrooveController controller_{};
    bool active_{false};
};

} // namespace lofibox::app
