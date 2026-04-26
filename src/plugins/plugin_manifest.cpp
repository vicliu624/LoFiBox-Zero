// SPDX-License-Identifier: GPL-3.0-or-later

#include "plugins/plugin_manifest.h"

#include <algorithm>

namespace lofibox::plugins {

void PluginRegistry::registerPlugin(PluginManifest manifest)
{
    if (findById(manifest.id) != nullptr) {
        return;
    }
    manifests_.push_back(std::move(manifest));
}

const std::vector<PluginManifest>& PluginRegistry::manifests() const noexcept
{
    return manifests_;
}

const PluginManifest* PluginRegistry::findById(const std::string& id) const noexcept
{
    const auto it = std::find_if(manifests_.begin(), manifests_.end(), [&id](const PluginManifest& manifest) {
        return manifest.id == id;
    });
    return it == manifests_.end() ? nullptr : &*it;
}

} // namespace lofibox::plugins
