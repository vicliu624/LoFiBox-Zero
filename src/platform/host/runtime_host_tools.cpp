// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_host_tools.h"

namespace lofibox::platform::host::runtime_detail {

std::string RuntimeTextParsing::trim(std::string value)
{
    return runtime_detail::trim(std::move(value));
}

std::string RuntimeTextParsing::asciiLower(std::string value)
{
    return runtime_detail::asciiLower(std::move(value));
}

std::string RuntimeTextParsing::repairMetadataText(std::string value)
{
    return runtime_detail::repairMetadataText(std::move(value));
}

std::vector<FilenameMetadataGuess> RuntimeTextParsing::fallbackMetadataGuessesFromPath(const fs::path& path)
{
    return runtime_detail::fallbackMetadataGuessesFromPath(path);
}

std::optional<std::string> RuntimeJsonTools::extractString(std::string_view json, std::string_view marker, std::size_t start)
{
    return extractJsonString(json, marker, start);
}

std::optional<bool> RuntimeJsonTools::parseBool(std::string_view json, std::string_view marker)
{
    return parseJsonBool(json, marker);
}

std::optional<int> RuntimeJsonTools::parseInt(std::string_view json, std::string_view marker)
{
    return parseJsonInt(json, marker);
}

std::string RuntimeJsonTools::escape(std::string_view value)
{
    return jsonEscape(value);
}

std::string RuntimeCachePaths::keyForPath(const fs::path& path)
{
    return cacheKeyForPath(path);
}

fs::path RuntimeCachePaths::metadataPath(const SharedRuntimeCache& cache, std::string_view key)
{
    return metadataCachePath(cache, key);
}

fs::path RuntimeCachePaths::artworkPath(const SharedRuntimeCache& cache, std::string_view key)
{
    return artworkCachePath(cache, key);
}

fs::path RuntimeCachePaths::lyricsPath(const SharedRuntimeCache& cache, std::string_view key)
{
    return lyricsCachePath(cache, key);
}

fs::path HelperScriptResolver::tagWriter()
{
    return tagWriterScriptPath();
}

fs::path HelperScriptResolver::remoteMediaTool()
{
    return remoteMediaToolScriptPath();
}

} // namespace lofibox::platform::host::runtime_detail
