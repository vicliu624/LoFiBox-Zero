// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_project_repository.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <sstream>
#include <utility>

namespace lofibox::groove {
namespace {

[[nodiscard]] std::filesystem::path envPath(const char* name)
{
#if defined(_MSC_VER)
    char* raw = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&raw, &size, name) != 0 || raw == nullptr || size == 0U) {
        if (raw != nullptr) {
            std::free(raw);
        }
        return {};
    }
    std::string value{raw};
    std::free(raw);
    return std::filesystem::path{value};
#else
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return {};
    }
    return std::filesystem::path{value};
#endif
}

[[nodiscard]] std::filesystem::path homePath()
{
    if (const auto home = envPath("HOME"); !home.empty()) {
        return home;
    }
    if (const auto profile = envPath("USERPROFILE"); !profile.empty()) {
        return profile;
    }
    return std::filesystem::current_path();
}

[[nodiscard]] std::filesystem::path xdgPath(const char* env_name, std::filesystem::path fallback)
{
    if (const auto configured = envPath(env_name); !configured.empty()) {
        return configured;
    }
    return fallback;
}

} // namespace

GrooveStoragePaths resolveGrooveStoragePaths()
{
    const auto home = homePath();
    const auto config_home = xdgPath("XDG_CONFIG_HOME", home / ".config");
    const auto data_home = xdgPath("XDG_DATA_HOME", home / ".local" / "share");
    const auto cache_home = xdgPath("XDG_CACHE_HOME", home / ".cache");

    GrooveStoragePaths paths{};
    paths.projectsDir = data_home / "lofibox" / "groove" / "projects";
    paths.samplesDir = data_home / "lofibox" / "groove" / "samples";
    paths.soundPacksDir = data_home / "lofibox" / "groove" / "soundpacks";
    paths.templatesDir = data_home / "lofibox" / "groove" / "templates";
    paths.cacheDir = cache_home / "lofibox" / "groove";
    paths.configFile = config_home / "lofibox" / "groove.json";
    paths.exportsDir = home / "Music" / "LoFiBox" / "Exports";
    paths.fallbackExportsDir = data_home / "lofibox" / "exports";
    return paths;
}

std::string sanitizeProjectFileName(std::string_view value)
{
    std::string out{};
    out.reserve(value.size());
    for (char ch : value) {
        const bool allowed = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_';
        out.push_back(allowed ? ch : '_');
    }
    if (out.empty()) {
        return "groove-project";
    }
    return out;
}

GrooveProjectRepository::GrooveProjectRepository()
    : paths_(resolveGrooveStoragePaths())
{}

GrooveProjectRepository::GrooveProjectRepository(GrooveStoragePaths paths)
    : paths_(std::move(paths))
{}

const GrooveStoragePaths& GrooveProjectRepository::paths() const noexcept
{
    return paths_;
}

bool GrooveProjectRepository::save(const GrooveProject& project, std::string* error) const
{
    try {
        std::filesystem::create_directories(paths_.projectsDir);
        std::ofstream file(projectPath(project.id), std::ios::binary | std::ios::trunc);
        if (!file) {
            if (error != nullptr) *error = "could not open project file for writing";
            return false;
        }
        const auto json = grooveProjectToJson(project);
        file.write(json.data(), static_cast<std::streamsize>(json.size()));
        return static_cast<bool>(file);
    } catch (const std::exception& ex) {
        if (error != nullptr) *error = ex.what();
        return false;
    }
}

bool GrooveProjectRepository::remove(std::string_view project_id, std::string* error) const
{
    try {
        return std::filesystem::remove(projectPath(project_id));
    } catch (const std::exception& ex) {
        if (error != nullptr) *error = ex.what();
        return false;
    }
}

GrooveProject GrooveProjectRepository::load(std::string_view project_id, std::string* error) const
{
    try {
        std::ifstream file(projectPath(project_id), std::ios::binary);
        if (!file) {
            if (error != nullptr) *error = "could not open project file";
            return makeDefaultGrooveProject();
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return grooveProjectFromJson(buffer.str());
    } catch (const std::exception& ex) {
        if (error != nullptr) *error = ex.what();
        return makeDefaultGrooveProject();
    }
}

std::vector<std::filesystem::path> GrooveProjectRepository::listProjectFiles() const
{
    std::vector<std::filesystem::path> files{};
    if (!std::filesystem::exists(paths_.projectsDir)) {
        return files;
    }
    for (const auto& entry : std::filesystem::directory_iterator(paths_.projectsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::filesystem::path GrooveProjectRepository::projectPath(std::string_view project_id) const
{
    return paths_.projectsDir / (sanitizeProjectFileName(project_id) + ".json");
}

} // namespace lofibox::groove
