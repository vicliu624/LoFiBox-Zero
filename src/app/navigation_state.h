// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "app/app_page.h"

namespace lofibox::app {

struct ListSelection {
    int selected{0};
    int scroll{0};
};

class NavigationState {
public:
    std::vector<AppPage> page_stack{AppPage::Boot};
    ListSelection list_selection{};

    [[nodiscard]] AppPage currentPage() const noexcept
    {
        return page_stack.empty() ? AppPage::Boot : page_stack.back();
    }

    void resetListSelection() noexcept
    {
        list_selection = {};
    }

    void pushPage(AppPage page)
    {
        page_stack.push_back(page);
        resetListSelection();
    }

    void replaceStack(std::vector<AppPage> pages)
    {
        page_stack = std::move(pages);
        resetListSelection();
    }

    void popPage()
    {
        if (page_stack.size() > 1) {
            page_stack.pop_back();
            resetListSelection();
        }
    }

    void clampListSelection(int count, int max_visible_rows)
    {
        if (count <= 0) {
            list_selection = {};
            return;
        }
        list_selection.selected = std::clamp(list_selection.selected, 0, count - 1);
        if (list_selection.selected < list_selection.scroll) {
            list_selection.scroll = list_selection.selected;
        }
        if (list_selection.selected >= list_selection.scroll + max_visible_rows) {
            list_selection.scroll = list_selection.selected - max_visible_rows + 1;
        }
        list_selection.scroll = std::clamp(list_selection.scroll, 0, std::max(0, count - max_visible_rows));
    }

    void moveSelection(int delta, int count, int max_visible_rows)
    {
        list_selection.selected += delta;
        clampListSelection(count, max_visible_rows);
    }
};

} // namespace lofibox::app
