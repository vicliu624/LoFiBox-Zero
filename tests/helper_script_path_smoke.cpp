// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_host_internal.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#if defined(_WIN32)
#include <cstdlib>
#endif

namespace fs = std::filesystem;
namespace runtime_detail = lofibox::platform::host::runtime_detail;

namespace {

bool setHelperDir(const fs::path& path)
{
#if defined(_WIN32)
    return _putenv_s("LOFIBOX_HELPER_DIR", path.string().c_str()) == 0;
#else
    return setenv("LOFIBOX_HELPER_DIR", path.string().c_str(), 1) == 0;
#endif
}

bool writeEmptyFile(const fs::path& path)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    return output.is_open();
}

} // namespace

int main()
{
    const fs::path root = fs::temp_directory_path() / "lofibox-helper-path-smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);
    if (ec) {
        std::cerr << "Failed to create helper test directory.\n";
        return 1;
    }

    const fs::path tag_writer = root / "tag_writer.py";
    const fs::path remote_tool = root / "remote_media_tool.py";
    if (!writeEmptyFile(tag_writer) || !writeEmptyFile(remote_tool)) {
        std::cerr << "Failed to create helper test files.\n";
        return 1;
    }

    if (!setHelperDir(root)) {
        std::cerr << "Failed to set LOFIBOX_HELPER_DIR.\n";
        return 1;
    }

    if (runtime_detail::tagWriterScriptPath() != tag_writer) {
        std::cerr << "tag writer helper path did not honor LOFIBOX_HELPER_DIR.\n";
        return 1;
    }
    if (runtime_detail::remoteMediaToolScriptPath() != remote_tool) {
        std::cerr << "remote media helper path did not honor LOFIBOX_HELPER_DIR.\n";
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
