// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "app/remote_media_services.h"

namespace lofibox::remote {

struct RemoteBrowseRequest {
    app::RemoteServerProfile profile{};
    app::RemoteSourceSession session{};
    app::RemoteCatalogNode parent{};
    int limit{50};
};

struct RemoteBrowseResult {
    std::vector<app::RemoteCatalogNode> nodes{};
    std::vector<app::RemoteTrack> tracks{};
    std::string source_explanation{};
};

class RemoteCatalogMap {
public:
    [[nodiscard]] static std::vector<app::RemoteCatalogNode> rootNodes();
    [[nodiscard]] static std::vector<app::RemoteCatalogNodeKind> supportedHierarchy();
    [[nodiscard]] static std::string explainNode(app::RemoteCatalogNodeKind kind);
};

} // namespace lofibox::remote
