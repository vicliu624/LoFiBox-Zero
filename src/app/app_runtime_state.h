// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "app/app_page.h"
#include "app/media_item.h"
#include "app/remote_media_services.h"
#include "app/app_state.h"
#include "app/navigation_state.h"
#include "ui/ui_models.h"

namespace lofibox::app {

struct AppRuntimeState {
    using clock = std::chrono::steady_clock;

    std::vector<std::filesystem::path> media_roots{};
    std::vector<std::string> pending_open_uris{};
    bool pending_open_processed{false};
    ui::UiAssets ui_assets{};
    clock::time_point boot_started{clock::now()};
    NavigationState navigation{};
    int main_menu_index{1};
    SettingsUiState settings{};
    NetworkState network{};
    MetadataServiceState metadata_service{};
    EqUiState eq{};
    std::vector<RemoteServerProfile> remote_profiles{};
    std::optional<RemoteServerKind> selected_remote_kind{};
    std::optional<std::size_t> selected_remote_profile_index{};
    std::optional<RemoteServerProfile> selected_transient_remote_profile{};
    RemoteSourceSession selected_remote_session{};
    RemoteCatalogNode selected_remote_parent{};
    std::vector<RemoteCatalogNode> remote_browse_nodes{};
    std::optional<RemoteCatalogNode> selected_remote_node{};
    std::optional<ResolvedRemoteStream> selected_remote_stream{};
    int remote_profile_edit_field{-1};
    std::string remote_profile_edit_buffer{};
    std::string remote_profile_status{};
    std::string search_query{};
    std::string search_results_query{};
    std::vector<MediaItem> search_results{};
    std::vector<std::string> search_degraded_sources{};
    clock::time_point last_update{clock::now()};
    clock::time_point last_status_refresh{};
    clock::time_point now_playing_confirm_blocked_until{};
    bool help_open{false};
    AppPage help_page{AppPage::MainMenu};
};

} // namespace lofibox::app
