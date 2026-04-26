// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

namespace lofibox::plugins {

enum class PluginKind {
    InternalProvider,
    BinaryProvider,
};

struct PluginManifest {
    std::string id{};
    std::string name{};
    PluginKind kind{PluginKind::InternalProvider};
    std::vector<std::string> capabilities{};
    std::vector<std::string> runtime_dependencies{};
};

class PluginRegistry {
public:
    void registerPlugin(PluginManifest manifest);
    [[nodiscard]] const std::vector<PluginManifest>& manifests() const noexcept;
    [[nodiscard]] const PluginManifest* findById(const std::string& id) const noexcept;

private:
    std::vector<PluginManifest> manifests_{};
};

} // namespace lofibox::plugins
