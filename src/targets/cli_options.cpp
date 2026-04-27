// SPDX-License-Identifier: GPL-3.0-or-later

#include "targets/cli_options.h"

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace lofibox::targets {

namespace {

#if defined(LOFIBOX_VERSION)
constexpr std::string_view kVersion{LOFIBOX_VERSION};
#else
constexpr std::string_view kVersion{"unknown"};
#endif

void printHelp(std::ostream& out)
{
    out << "LoFiBox " << kVersion << '\n'
        << '\n'
        << "Usage:\n"
        << "  lofibox [options] [audio-file-or-url...]\n"
        << '\n'
        << "Options:\n"
        << "  --help        Show this help text and exit.\n"
        << "  --version     Show version information and exit.\n"
        << '\n'
        << "Environment:\n"
        << "  LOFIBOX_ASSET_DIR   Override the runtime asset directory.\n"
        << "  LOFIBOX_FONT_PATH   Override the system font file used for text rendering.\n";
}

} // namespace

bool handleCommonCliOptions(int argc, char** argv, std::ostream& out)
{
    for (int index = 1; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--help" || current == "-h") {
            printHelp(out);
            return true;
        }
        if (current == "--version") {
            out << "lofibox " << kVersion << '\n';
            return true;
        }
    }

    return false;
}

std::vector<std::string> positionalOpenUris(int argc, char** argv)
{
    std::vector<std::string> uris{};
    for (int index = 1; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--help" || current == "-h" || current == "--version") {
            continue;
        }
        if ((current == "--fbdev" || current == "--input-dev") && (index + 1) < argc) {
            ++index;
            continue;
        }
        if (!current.empty() && current.front() == '-') {
            continue;
        }
        uris.emplace_back(current);
    }
    return uris;
}

} // namespace lofibox::targets
