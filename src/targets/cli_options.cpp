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

bool hasOption(int argc, char** argv, std::string_view option)
{
    for (int index = 1; index < argc; ++index) {
        if (std::string_view{argv[index]} == option) {
            return true;
        }
    }
    return false;
}

std::string_view firstCommand(int argc, char** argv)
{
    for (int index = 1; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--runtime-socket" || current == "--fields") {
            ++index;
            continue;
        }
        if (!current.empty() && current.front() == '-') {
            continue;
        }
        return current;
    }
    return {};
}

void printVersion(std::ostream& out, int argc, char** argv)
{
    if (hasOption(argc, argv, "--quiet")) {
        return;
    }
    if (hasOption(argc, argv, "--json")) {
        out << "{\"name\":\"lofibox\",\"version\":\"" << kVersion << "\"}\n";
        return;
    }
    if (hasOption(argc, argv, "--porcelain")) {
        out << "version\t" << kVersion << '\n';
        return;
    }
    out << "lofibox " << kVersion << '\n';
}

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
        << "  --json        Emit machine-readable JSON for supported state commands.\n"
        << "  --porcelain   Emit stable tab-separated output for supported state commands.\n"
        << "  --fields a,b  Limit named fields where supported.\n"
        << "  --quiet       Suppress stdout where supported.\n"
        << '\n'
        << "Direct commands:\n"
        << "  version\n"
        << "  source list|show|add|update|remove|probe|auth-status|capabilities\n"
        << "  credentials list|show-ref|set|status|validate|delete\n"
        << "  library scan|status|list|query\n"
        << "  search <text>\n"
        << "  remote browse|recent|search|resolve|stream-info\n"
        << "  cache status|purge|gc\n"
        << "  doctor [--json]\n"
        << '\n'
        << "Runtime commands (requires a running LoFiBox instance):\n"
        << "  now\n"
        << "  runtime status|playback|queue|eq|remote|settings\n"
        << "  play [--id <track-id>|--pause|--resume|--toggle|--next|--prev|--stop|--seek <seconds>|--shuffle on|off|--repeat off|one|all]\n"
        << "  pause|resume|toggle|stop|seek <seconds>|next|prev\n"
        << "  queue show|set <index>|next|clear\n"
        << "  shuffle on|off|repeat off|all|one\n"
        << "  eq show|enable|disable|preset <name>|band <index> <gain>\n"
        << "  remote reconnect|runtime reload|runtime shutdown\n"
        << '\n'
        << "Environment:\n"
        << "  LOFIBOX_ASSET_DIR   Override the runtime asset directory.\n"
        << "  LOFIBOX_FONT_PATH   Override the system font file used for text rendering.\n";
}

} // namespace

bool handleCommonCliOptions(int argc, char** argv, std::ostream& out)
{
    if (firstCommand(argc, argv) == "version") {
        printVersion(out, argc, argv);
        return true;
    }

    for (int index = 1; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--help" || current == "-h") {
            printHelp(out);
            return true;
        }
        if (current == "--version") {
            printVersion(out, argc, argv);
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
