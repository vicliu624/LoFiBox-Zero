// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/widgets/tui_queue_view.h"

#include <iomanip>
#include <sstream>

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::widgets {

std::vector<std::string> renderQueueView(const runtime::QueueRuntimeSnapshot& queue, int max_lines, int width)
{
    std::vector<std::string> lines{};
    if (max_lines <= 0) {
        return lines;
    }
    if (queue.visible_items.empty()) {
        lines.push_back("Queue empty");
        return lines;
    }
    for (const auto& item : queue.visible_items) {
        if (static_cast<int>(lines.size()) >= max_lines) {
            break;
        }
        std::ostringstream row{};
        row << (item.active ? "> " : "  ")
            << std::setw(2) << std::setfill('0') << (item.queue_index + 1) << ' '
            << (item.title.empty() ? "Unknown" : item.title);
        if (!item.artist.empty()) {
            row << " - " << item.artist;
        }
        lines.push_back(fitText(row.str(), width));
    }
    return lines;
}

} // namespace lofibox::tui::widgets

