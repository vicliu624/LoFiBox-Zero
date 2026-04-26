// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <string>

#include "app/input_event.h"
#include "core/canvas.h"

namespace lofibox::app {

class HelloApp {
public:
    HelloApp();

    void handleInput(const InputEvent& event) noexcept;
    void render(core::Canvas& canvas) const noexcept;

private:
    [[nodiscard]] bool blinkOn() const noexcept;
    [[nodiscard]] bool hasTypedText() const noexcept;
    [[nodiscard]] std::string visibleInput() const;
    [[nodiscard]] std::string visibleStatus() const;
    [[nodiscard]] std::string visibleLastKey() const;

    std::chrono::steady_clock::time_point started_at_{};
    std::string typed_text_{};
    std::string last_key_label_{"NONE"};
    std::string status_text_{"CLICK DEVICE KEYS OR TYPE ON PC"};
};

} // namespace lofibox::app
