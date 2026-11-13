// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace lofibox::core {

struct PlaybackInput {
    std::string value{};
    bool remote{false};
};

inline std::string pathUtf8(const std::filesystem::path& path)
{
    const auto bytes = path.u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

inline bool supportedAudioFile(const std::filesystem::path& path)
{
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
    return extension == ".mp3" || extension == ".wav" || extension == ".aac" || extension == ".flac"
        || extension == ".ogg" || extension == ".opus" || extension == ".alac" || extension == ".ape" || extension == ".aiff";
}

// Resolve local input at the submitting process, before crossing the runtime
// socket. The server also validates it before replacing the active track.
inline std::optional<PlaybackInput> normalizePlaybackInput(std::string input)
{
    if (input.empty() || input.find('\0') != std::string::npos) return std::nullopt;
    auto lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    if (lower.rfind("file:", 0) == 0) {
        std::string_view path{input};
        path.remove_prefix(5);
        if (path.rfind("//", 0) == 0) {
            path.remove_prefix(2);
            const auto slash = path.find('/');
            if (slash == std::string_view::npos) return std::nullopt;
            auto authority = std::string{path.substr(0, slash)};
            std::transform(authority.begin(), authority.end(), authority.begin(), [](unsigned char c) { return std::tolower(c); });
            if (!authority.empty() && authority != "localhost") return std::nullopt;
            path.remove_prefix(slash);
        }
        if (path.empty() || path.front() != '/' || path.find_first_of("?#") != std::string_view::npos) return std::nullopt;
        std::string decoded;
        const auto hex = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        for (std::size_t i = 0; i < path.size(); ++i) {
            if (path[i] != '%') { decoded.push_back(path[i]); continue; }
            if (i + 2 >= path.size()) return std::nullopt;
            const int a = hex(path[i + 1]), b = hex(path[i + 2]);
            if (a < 0 || b < 0 || (a == 0 && b == 0)) return std::nullopt;
            decoded.push_back(static_cast<char>((a << 4) | b));
            i += 2;
        }
        input = std::move(decoded);
    } else if (const auto scheme_end = input.find("://"); scheme_end != std::string::npos) {
        const auto scheme = lower.substr(0, scheme_end);
        if (scheme != "http" && scheme != "https" && scheme != "ftp" && scheme != "sftp"
            && scheme != "smb" && scheme != "nfs" && scheme != "webdav" && scheme != "webdavs") return std::nullopt;
        const auto authority_end = input.find_first_of("/?#", scheme_end + 3);
        const auto authority = input.substr(scheme_end + 3, authority_end == std::string::npos ? std::string::npos : authority_end - scheme_end - 3);
        const auto at = authority.rfind('@');
        const auto host = authority.substr(at == std::string::npos ? 0 : at + 1);
        if (host.empty() || host.front() == ':' || std::any_of(input.begin(), input.end(), [](unsigned char c) { return c <= 0x20 || c == 0x7f; })) return std::nullopt;
        // FFmpeg consumes HTTP URLs for WebDAV resources; browsing/authentication
        // through a configured provider remains a separate source operation.
        input.replace(0, scheme_end, scheme == "webdav" ? "http" : scheme == "webdavs" ? "https" : scheme);
        return PlaybackInput{std::move(input), true};
    }
    std::error_code ec;
    auto path = std::filesystem::canonical(std::filesystem::u8path(input), ec);
    if (ec || !supportedAudioFile(path) || !std::filesystem::is_regular_file(path, ec) || ec) return std::nullopt;
    return PlaybackInput{pathUtf8(path), false};
}

// Display only the resource identity, without userinfo, query tokens or fragments.
inline std::string playbackInputDisplayUri(const std::string& input)
{
    auto visible = input.substr(0, input.find_first_of("?#"));
    const auto scheme = visible.find("://");
    if (scheme != std::string::npos) {
        const auto slash = visible.find('/', scheme + 3);
        const auto at = visible.rfind('@', slash == std::string::npos ? visible.size() : slash);
        if (at != std::string::npos && at >= scheme + 3) visible.erase(scheme + 3, at - scheme - 2);
    }
    return visible;
}

} // namespace lofibox::core
