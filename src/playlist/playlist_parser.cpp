// SPDX-License-Identifier: GPL-3.0-or-later

#include "playlist/playlist_parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace lofibox::playlist {
namespace {

std::string lowerExtension(std::filesystem::path path)
{
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension;
}

std::string trim(std::string text)
{
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

} // namespace

PlaylistFormat detectPlaylistFormat(const std::filesystem::path& path)
{
    const auto extension = lowerExtension(path);
    if (extension == ".m3u" || extension == ".m3u8") {
        return PlaylistFormat::M3u;
    }
    if (extension == ".pls") {
        return PlaylistFormat::Pls;
    }
    if (extension == ".xspf") {
        return PlaylistFormat::Xspf;
    }
    return PlaylistFormat::Unknown;
}

PlaylistDocument parseM3u(std::string_view text)
{
    PlaylistDocument document{};
    document.format = PlaylistFormat::M3u;

    std::istringstream input{std::string(text)};
    std::string line{};
    std::string pending_title{};
    int pending_duration = -1;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        if (line.rfind("#EXTINF:", 0) == 0) {
            const auto comma = line.find(',');
            if (comma != std::string::npos) {
                pending_title = trim(line.substr(comma + 1));
                const auto duration = trim(line.substr(8, comma - 8));
                try {
                    pending_duration = std::stoi(duration);
                } catch (...) {
                    pending_duration = -1;
                }
            }
            continue;
        }
        if (line[0] == '#') {
            continue;
        }
        document.entries.push_back({line, pending_title, pending_duration});
        pending_title.clear();
        pending_duration = -1;
    }
    return document;
}

} // namespace lofibox::playlist
