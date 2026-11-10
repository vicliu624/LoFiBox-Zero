// SPDX-License-Identifier: GPL-3.0-or-later
#include "tui/tui_input_router.h"
#include "app/text_input_utils.h"
#include <iostream>
#include <string>
#include <vector>

int main()
{
    using namespace lofibox::tui;
    const std::vector<std::string> prefixes{
        "", "\xe6\xb1\x89", "\xe6\xb1\x89\xc3\xa9", "\xe6\xb1\x89\xc3\xa9\xf0\x9f\x8e\xb5",
        "\xe6\xb1\x89\xc3\xa9\xf0\x9f\x8e\xb5" "A"};
    for (auto mode : {TuiInputMode::Search,TuiInputMode::Command,TuiInputMode::Text}) {
        auto model = makeTuiModel({}); model.input_mode = mode;
        (void)routeTuiInput(model, TuiKeyEvent{TuiKey::Paste, 0, false, false, false, prefixes.back()});
        auto& value = mode == TuiInputMode::Command ? model.command_buffer : model.search_buffer;
        for (int index = static_cast<int>(prefixes.size())-2; index >= 0; --index) {
            const auto result = routeTuiInput(model, TuiKeyEvent{TuiKey::Backspace});
            if (value != prefixes[static_cast<std::size_t>(index)] || !lofibox::app::isValidUtf8(value)
                || result.command || model.input_mode != mode) {
                std::cerr << "Backspace corrupted committed UTF-8 or emitted a command at prefix " << index << '\n';
                return 1;
            }
        }
        (void)routeTuiInput(model, TuiKeyEvent{TuiKey::Backspace});
        if (!value.empty()) return 2;
    }
    std::cout << "Mixed ASCII, two/three/four-byte text deletion passed in all TUI text modes.\n";
}
