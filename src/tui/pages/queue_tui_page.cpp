// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/pages/queue_tui_page.h"

#include "tui/widgets/tui_queue_view.h"
#include "tui/widgets/tui_text.h"

namespace lofibox::tui::pages {

std::vector<std::string> queuePageLines(const TuiModel& model, const TuiLayout& layout)
{
    std::vector<std::string> lines{};
    lines.push_back(widgets::fitText("Queue · index " + std::to_string(model.snapshot.queue.active_index), layout.size.width));
    for (const auto& line : widgets::renderQueueView(model.snapshot.queue, layout.size.height - 2, layout.size.width)) {
        lines.push_back(line);
    }
    return lines;
}

} // namespace lofibox::tui::pages

