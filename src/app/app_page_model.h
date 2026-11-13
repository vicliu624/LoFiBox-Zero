// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "app/app_page.h"
#include "app/app_state.h"
#include "app/runtime_preferences.h"
#include "core/display_profile.h"

namespace lofibox::app {

inline constexpr int kPageModelMaxVisibleRows = core::kListVisibleRows;

struct AppPageModelInput {
    AppPage page{AppPage::Boot};
    std::optional<std::string> library_title_override{};
    std::optional<std::vector<std::pair<std::string, std::string>>> library_rows{};
    SettingsState settings{};
    bool network_connected{false};
    std::string metadata_display_name{};
    std::string webui_url{};
    std::optional<std::vector<std::pair<std::string, std::string>>> source_manager_rows{};
    std::optional<std::vector<std::pair<std::string, std::string>>> page_rows{};
    std::uint32_t preference_flags{RuntimePreferences::kDefaults};
    bool device_lights{false};
    bool preferences_error{false};
};

struct AppPageModel {
    std::string title{};
    std::vector<std::pair<std::string, std::string>> rows{};
    bool browse_list{false};
    int max_visible_rows{kPageModelMaxVisibleRows};
    // A page may project only the visible portion of a large shared list.
    // Negative total_rows retains the ordinary complete-vector contract.
    int total_rows{-1};
    int row_offset{0};
    [[nodiscard]] int rowCount() const noexcept
    {
        return total_rows >= 0 ? total_rows : static_cast<int>(rows.size());
    }
};

[[nodiscard]] AppPageModel buildAppPageModel(const AppPageModelInput& input);
[[nodiscard]] bool isBrowseListPage(AppPage page) noexcept;

} // namespace lofibox::app
