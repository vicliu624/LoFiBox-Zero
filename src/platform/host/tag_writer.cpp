// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_provider_factories.h"

#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "platform/host/png_canvas_loader.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;
class PythonMutagenTagWriter final : public app::TagWriter {
public:
    PythonMutagenTagWriter()
    {
        python_path_ = resolvePythonPath();
    }

    [[nodiscard]] bool available() const override
    {
        return unavailableReason().empty();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "MUTAGEN";
    }

    bool write(const fs::path& path, const app::TagWriteRequest& request) const override
    {
        if (!available()) {
            logRuntime(RuntimeLogLevel::Warn, "tagwriter", "Tag writer unavailable: " + unavailableReason() + " for " + pathUtf8String(path));
            return false;
        }
        const bool ok = writeTagPayload(*python_path_, path, request);
        logRuntime(ok ? RuntimeLogLevel::Info : RuntimeLogLevel::Warn, "tagwriter", std::string(ok ? "Writeback succeeded for " : "Writeback failed for ") + pathUtf8String(path));
        return ok;
    }

private:
    [[nodiscard]] std::string unavailableReason() const
    {
        if (!python_path_.has_value()) {
            return "python3 executable not found";
        }
        if (!fs::exists(tagWriterScriptPath())) {
            return "tag_writer.py helper not found at " + pathUtf8String(tagWriterScriptPath());
        }
        if (!runProcess(*python_path_, {"-c", "import mutagen"})) {
            return "python3-mutagen module not available";
        }
        return {};
    }

    std::optional<fs::path> python_path_{};
};

} // namespace

std::shared_ptr<app::TagWriter> createHostTagWriter()
{
    return std::make_shared<PythonMutagenTagWriter>();
}

} // namespace lofibox::platform::host
