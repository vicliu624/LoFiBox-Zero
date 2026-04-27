// SPDX-License-Identifier: GPL-3.0-or-later

#include "remote/common/stream_source_model.h"

#include <algorithm>
#include <cctype>

namespace lofibox::remote {
namespace {

bool endsWith(const std::string& value, const std::string& suffix)
{
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

} // namespace

DirectStreamEntry StreamSourceClassifier::classify(std::string uri)
{
    const auto normalized = lower(uri);
    DirectStreamEntry entry{};
    entry.uri = std::move(uri);
    if (normalized.rfind("https://", 0) == 0) {
        entry.protocol = app::StreamProtocol::Https;
    } else if (normalized.rfind("http://", 0) == 0) {
        entry.protocol = app::StreamProtocol::Http;
    } else if (normalized.rfind("smb://", 0) == 0) {
        entry.protocol = app::StreamProtocol::Smb;
    } else if (normalized.rfind("nfs://", 0) == 0) {
        entry.protocol = app::StreamProtocol::Nfs;
    } else if (normalized.rfind("webdav://", 0) == 0 || normalized.rfind("webdavs://", 0) == 0) {
        entry.protocol = app::StreamProtocol::WebDav;
    } else if (normalized.rfind("ftp://", 0) == 0) {
        entry.protocol = app::StreamProtocol::Ftp;
    } else if (normalized.rfind("sftp://", 0) == 0) {
        entry.protocol = app::StreamProtocol::Sftp;
    } else if (normalized.rfind("dlna://", 0) == 0 || normalized.rfind("upnp://", 0) == 0) {
        entry.protocol = app::StreamProtocol::DlnaUpnp;
    } else {
        entry.protocol = app::StreamProtocol::File;
    }

    if (endsWith(normalized, ".m3u")) {
        entry.playlist_format = PlaylistManifestFormat::M3u;
    } else if (endsWith(normalized, ".m3u8")) {
        entry.playlist_format = PlaylistManifestFormat::M3u8;
        entry.protocol = app::StreamProtocol::Hls;
        entry.adaptive = true;
    } else if (endsWith(normalized, ".pls")) {
        entry.playlist_format = PlaylistManifestFormat::Pls;
    } else if (endsWith(normalized, ".xspf")) {
        entry.playlist_format = PlaylistManifestFormat::Xspf;
    }
    if (normalized.find("icecast") != std::string::npos || normalized.find("shoutcast") != std::string::npos) {
        entry.radio = true;
    }
    if (endsWith(normalized, ".mpd")) {
        entry.protocol = app::StreamProtocol::Dash;
        entry.adaptive = true;
    }
    return entry;
}

bool StreamSourceClassifier::supportedProtocol(app::StreamProtocol protocol) noexcept
{
    switch (protocol) {
    case app::StreamProtocol::File:
    case app::StreamProtocol::Http:
    case app::StreamProtocol::Https:
    case app::StreamProtocol::Hls:
    case app::StreamProtocol::Dash:
    case app::StreamProtocol::Icecast:
    case app::StreamProtocol::Shoutcast:
    case app::StreamProtocol::Smb:
    case app::StreamProtocol::Nfs:
    case app::StreamProtocol::WebDav:
    case app::StreamProtocol::Ftp:
    case app::StreamProtocol::Sftp:
    case app::StreamProtocol::DlnaUpnp:
        return true;
    }
    return false;
}

std::string StreamSourceClassifier::redactedUri(std::string uri)
{
    const auto scheme = uri.find("://");
    if (scheme == std::string::npos) {
        return uri;
    }
    const auto at = uri.find('@', scheme + 3);
    if (at == std::string::npos) {
        return uri;
    }
    return uri.substr(0, scheme + 3) + "***:***@" + uri.substr(at + 1);
}

} // namespace lofibox::remote
