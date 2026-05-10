// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "groove/groove_project.h"

namespace lofibox::groove {

struct GrooveStoragePaths {
    std::filesystem::path projectsDir{};
    std::filesystem::path samplesDir{};
    std::filesystem::path soundPacksDir{};
    std::filesystem::path templatesDir{};
    std::filesystem::path cacheDir{};
    std::filesystem::path configFile{};
    std::filesystem::path exportsDir{};
    std::filesystem::path fallbackExportsDir{};
};

[[nodiscard]] GrooveStoragePaths resolveGrooveStoragePaths();
[[nodiscard]] std::string sanitizeProjectFileName(std::string_view value);

class GrooveProjectRepository {
public:
    GrooveProjectRepository();
    explicit GrooveProjectRepository(GrooveStoragePaths paths);

    [[nodiscard]] const GrooveStoragePaths& paths() const noexcept;
    [[nodiscard]] bool save(const GrooveProject& project, std::string* error = nullptr) const;
    [[nodiscard]] bool remove(std::string_view project_id, std::string* error = nullptr) const;
    [[nodiscard]] GrooveProject load(std::string_view project_id, std::string* error = nullptr) const;
    [[nodiscard]] std::vector<std::filesystem::path> listProjectFiles() const;

private:
    [[nodiscard]] std::filesystem::path projectPath(std::string_view project_id) const;

    GrooveStoragePaths paths_{};
};

} // namespace lofibox::groove
