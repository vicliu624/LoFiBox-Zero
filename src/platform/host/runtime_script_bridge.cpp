// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_host_internal.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "platform/host/runtime_logger.h"
#include "platform/host/runtime_paths.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {

std::optional<fs::path> resolvePythonPath()
{
#if defined(_WIN32)
    if (const auto python = resolveExecutablePath(L"PYTHON_EXECUTABLE", L"python.exe")) {
        return python;
    }
    return resolveExecutablePath(nullptr, L"py.exe");
#elif defined(__linux__)
    return resolveExecutablePath("PYTHON_EXECUTABLE", "python3");
#else
    return std::nullopt;
#endif
}

bool writeTagPayload(
    const fs::path& python_path,
    const fs::path& target_path,
    const app::TagWriteRequest& request)
{
    const fs::path payload_path = runtime_paths::appTemporaryDir() / ("tag-write-" + cacheKeyForPath(target_path) + ".json");
    std::error_code directory_error{};
    fs::create_directories(payload_path.parent_path(), directory_error);
    std::ofstream output(payload_path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << "{\n";
    output << "  \"path\": \"" << jsonEscape(pathUtf8String(target_path)) << "\",\n";
    output << "  \"only_fill_missing\": " << (request.only_fill_missing ? "true" : "false") << ",\n";
    output << "  \"clear_missing_metadata\": " << (request.clear_missing_metadata ? "true" : "false") << ",\n";
    if (request.metadata) {
        output << "  \"metadata\": {\n";
        auto emitField = [&](std::string_view name, const std::optional<std::string>& value, bool trailing) {
            output << "    \"" << name << "\": ";
            if (value) {
                output << "\"" << jsonEscape(*value) << "\"";
            } else {
                output << "null";
            }
            output << (trailing ? ",\n" : "\n");
        };
        emitField("title", request.metadata->title, true);
        emitField("artist", request.metadata->artist, true);
        emitField("album", request.metadata->album, true);
        emitField("genre", request.metadata->genre, true);
        emitField("composer", request.metadata->composer, true);
        output << "    \"duration_seconds\": ";
        if (request.metadata->duration_seconds) {
            output << *request.metadata->duration_seconds << "\n";
        } else {
            output << "null\n";
        }
        output << "  },\n";
    } else {
        output << "  \"metadata\": null,\n";
    }

    if (request.identity) {
        output << "  \"identity\": {\n";
        output << "    \"recording_mbid\": \"" << jsonEscape(request.identity->recording_mbid) << "\",\n";
        output << "    \"release_mbid\": \"" << jsonEscape(request.identity->release_mbid) << "\",\n";
        output << "    \"release_group_mbid\": \"" << jsonEscape(request.identity->release_group_mbid) << "\",\n";
        output << "    \"source\": \"" << jsonEscape(request.identity->source) << "\",\n";
        output << "    \"confidence\": " << request.identity->confidence << ",\n";
        output << "    \"audio_fingerprint_verified\": " << (request.identity->audio_fingerprint_verified ? "true" : "false") << "\n";
        output << "  },\n";
    } else {
        output << "  \"identity\": null,\n";
    }

    output << "  \"artwork_png_path\": ";
    if (request.artwork_png_path) {
        output << "\"" << jsonEscape(pathUtf8String(*request.artwork_png_path)) << "\",\n";
    } else {
        output << "null,\n";
    }

    if (request.lyrics) {
        output << "  \"lyrics\": {\n";
        output << "    \"plain\": " << (request.lyrics->plain ? "\"" + jsonEscape(*request.lyrics->plain) + "\"" : "null") << ",\n";
        output << "    \"synced\": " << (request.lyrics->synced ? "\"" + jsonEscape(*request.lyrics->synced) + "\"" : "null") << ",\n";
        output << "    \"source\": \"" << jsonEscape(request.lyrics->source) << "\"\n";
        output << "  }\n";
    } else {
        output << "  \"lyrics\": null\n";
    }
    output << "}\n";
    output.close();

    const std::vector<std::string> args{pathUtf8String(tagWriterScriptPath()), pathUtf8String(payload_path)};
    const bool ok = runProcess(python_path, args);
    std::error_code ec{};
    fs::remove(payload_path, ec);
    return ok;
}

std::optional<std::string> callRemoteMediaTool(const fs::path& python_path, const std::string& payload_json)
{
    const fs::path payload_path = runtime_paths::appTemporaryDir() / ("remote-media-" + std::to_string(std::hash<std::string>{}(payload_json)) + ".json");
    std::error_code directory_error{};
    fs::create_directories(payload_path.parent_path(), directory_error);
    std::ofstream output(payload_path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        logRuntime(RuntimeLogLevel::Warn, "remote", "Failed to create remote-media payload file");
        return std::nullopt;
    }
    output << payload_json;
    output.close();

    const auto result = captureProcessOutput(python_path, {pathUtf8String(remoteMediaToolScriptPath()), pathUtf8String(payload_path)});
    std::error_code ec{};
    fs::remove(payload_path, ec);
    if (!result) {
        logRuntime(RuntimeLogLevel::Warn, "remote", "Remote media tool failed");
    }
    return result;
}


} // namespace lofibox::platform::host::runtime_detail
