// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>

#include "app/remote_media_services.h"

namespace lofibox::remote {

enum class PlaylistManifestFormat {
    M3u,
    M3u8,
    Pls,
    Xspf,
    Unknown,
};

struct DirectStreamEntry {
    std::string uri{};
    app::StreamProtocol protocol{app::StreamProtocol::Http};
    PlaylistManifestFormat playlist_format{PlaylistManifestFormat::Unknown};
    bool radio{false};
    bool adaptive{false};
};

class StreamSourceClassifier {
public:
    [[nodiscard]] static DirectStreamEntry classify(std::string uri);
    [[nodiscard]] static bool supportedProtocol(app::StreamProtocol protocol) noexcept;
    [[nodiscard]] static std::string redactedUri(std::string uri);
};

} // namespace lofibox::remote
