// SPDX-License-Identifier: GPL-3.0-or-later

#include "targets/cli_options.h"

#include "cli/cli_format.h"

#include <initializer_list>
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

struct CommandSchema {
    std::string_view command{};
    std::string_view kind{};
    bool requires_runtime{false};
    bool mutates{false};
    std::string_view summary{};
    std::string_view usage{};
    std::vector<std::string_view> positionals{};
    std::vector<std::string_view> options{};
    std::vector<std::string_view> fields{};
    std::vector<std::string_view> examples{};
    std::vector<std::string_view> safety{};
    std::vector<std::string_view> recovery{};
};

void printJsonStringArray(std::ostream& out, std::string_view name, const std::vector<std::string_view>& values, bool& first)
{
    if (!first) {
        out << ',';
    }
    first = false;
    cli::printJsonString(out, name);
    out << ":[";
    bool first_value = true;
    for (const auto value : values) {
        if (!first_value) {
            out << ',';
        }
        first_value = false;
        cli::printJsonString(out, value);
    }
    out << ']';
}

std::vector<CommandSchema> commandSchemas()
{
    return {
        {"version", "query", false, false,
            "Show the installed executable version.",
            "lofibox version [--json|--porcelain|--quiet]",
            {},
            {"--json", "--porcelain", "--quiet"},
            {"name", "version"},
            {"lofibox version", "lofibox version --json"},
            {},
            {"If package behavior looks stale, compare this output with dpkg -s lofibox."}},
        {"doctor", "query", false, false,
            "Diagnose host paths, helper tools, credential store, sources, and runtime readiness.",
            "lofibox doctor [--json|--porcelain|--quiet]",
            {},
            {"--json", "--porcelain", "--quiet"},
            {"status", "degraded", "capabilities", "sources"},
            {"lofibox doctor", "lofibox doctor --json"},
            {},
            {"Install missing helpers, fix XDG path permissions, then rerun doctor."}},
        {"source", "direct", false, true,
            "Manage durable remote/source profiles. Profiles store non-secret connection facts and credential references.",
            "lofibox source list|show|add|update|remove|probe|auth-status|capabilities ...",
            {"list", "show <profile-id>", "add <kind>", "update <profile-id>", "remove <profile-id>", "probe <profile-id>", "auth-status <profile-id>", "capabilities <profile-id>"},
            {"--id <id>", "--name <label>", "--base-url <url>", "--username <user>", "--credential-ref <ref>", "--verify-peer true|false", "--allow-self-signed true|false", "--json", "--porcelain", "--fields"},
            {"id", "kind", "name", "base_url", "username", "credential_ref", "readiness", "permission", "verify_peer", "allow_self_signed"},
            {"lofibox source list --json", "lofibox source add jellyfin --id home --name \"Home Jellyfin\" --base-url https://media.local:8096 --username vicliu --credential-ref jellyfin-home", "lofibox source probe home"},
            {"Does not print raw passwords or tokens.", "Use credentials set --password-stdin or --token-stdin for secrets."},
            {"If probe fails, run source show, source auth-status, credentials validate, and doctor --json."}},
        {"credentials", "direct", false, true,
            "Manage credential refs and secret status without echoing raw secret material.",
            "lofibox credentials list|show-ref|set|status|validate|delete ...",
            {"list", "show-ref <profile-id-or-ref>", "set <profile-id-or-ref>", "status <profile-id-or-ref>", "validate <profile-id-or-ref>", "delete <profile-id-or-ref>"},
            {"--username <user>", "--password-stdin", "--token-stdin", "--json", "--porcelain", "--fields"},
            {"profile_id", "credential_ref", "username", "password", "token"},
            {"printf '%s\\n' \"$JELLYFIN_TOKEN\" | lofibox credentials set jellyfin-home --token-stdin", "lofibox credentials show-ref jellyfin-home --json"},
            {"Do not pass secrets as argv values; they leak through shell history and process listings."},
            {"If auth fails, update the secret through stdin and run source probe again."}},
        {"library", "direct", false, true,
            "Scan and query the local media library index.",
            "lofibox library scan|status|list|query ...",
            {"scan [path...]", "status [--root <path>]", "list tracks|albums|artists|genres|composers|compilations", "query tracks|albums|<text>"},
            {"--root <path>", "--artist <name>", "--album-id <id>", "--genre <name>", "--recently-added", "--most-played", "--recently-played", "--json", "--porcelain", "--fields"},
            {"id", "title", "artist", "album", "genre", "composer", "duration", "play_count", "added_time", "last_played", "path"},
            {"lofibox library scan ~/Music", "lofibox library list tracks --json", "lofibox library query tracks --artist \"Aimer\""},
            {},
            {"If the library is empty, run library status --json and verify the root exists."}},
        {"search", "direct", false, false,
            "Search local and configured remote sources with grouped MediaItem semantics.",
            "lofibox search <text> [--local|--source <profile-id>|--all]",
            {"<text>"},
            {"--local", "--source <profile-id>", "--all", "--root <path>", "--json", "--porcelain", "--fields"},
            {"group", "id", "title", "artist", "album", "source"},
            {"lofibox search \"hotel california\" --local", "lofibox search \"utada\" --source jellyfin-home --json"},
            {},
            {"If remote search is empty, run source probe and remote browse first."}},
        {"remote", "direct", false, false,
            "Browse, search, resolve, and inspect stream diagnostics for configured remote catalogs.",
            "lofibox remote browse|recent|search|resolve|stream-info <profile-id> ...",
            {"browse <profile-id> [/artists|/albums|/playlists|/stations]", "recent <profile-id>", "search <profile-id> <text>", "resolve <profile-id> <item-id>", "stream-info <profile-id> <item-id>"},
            {"--json", "--porcelain", "--fields"},
            {"kind", "id", "title", "subtitle", "playable", "browsable", "artist", "album", "duration", "profile_id", "item_id", "resolved", "url", "connection", "buffer", "bitrate", "codec", "live", "seekable"},
            {"lofibox remote browse jellyfin-home /albums", "lofibox remote search jellyfin-home radiohead", "lofibox remote stream-info jellyfin-home 12345 --json"},
            {"Resolved URLs are redacted in CLI output."},
            {"If resolve fails, run source auth-status and source capabilities."}},
        {"metadata", "direct", false, true,
            "Read local tags or trigger governed online metadata enrichment for a local audio file.",
            "lofibox metadata show|lookup|apply <path>",
            {"show <path>", "lookup <path>", "apply <path>"},
            {"--json", "--porcelain", "--fields"},
            {"status", "path", "provider", "title", "artist", "album", "genre", "composer", "duration", "found"},
            {"lofibox metadata show song.flac --json", "lofibox metadata lookup song.flac", "lofibox metadata apply song.flac"},
            {"apply may write missing accepted tags when the writeback policy allows it."},
            {"If lookup misses, inspect doctor metadata helpers and runtime logs for metadata provider reasons."}},
        {"lyrics", "direct", false, true,
            "Read embedded/cache lyrics or trigger governed lyrics lookup and safe writeback.",
            "lofibox lyrics show|lookup|apply <path>",
            {"show <path>", "lookup <path>", "apply <path>"},
            {"--json", "--porcelain", "--fields"},
            {"status", "path", "provider", "source", "plain_available", "synced_available", "plain_lines", "synced_lines", "plain_preview", "synced_preview"},
            {"lofibox lyrics show song.flac --json", "lofibox lyrics lookup song.flac", "lofibox lyrics apply song.flac"},
            {"apply writes lyrics only when identity-aware policy accepts the match."},
            {"If lyrics are missing in GUI/TUI, run lyrics lookup and then now --fields lyrics.available,lyrics.status --json while playing."}},
        {"artwork", "direct", false, true,
            "Inspect or trigger governed artwork lookup for a local file.",
            "lofibox artwork show|lookup <path>",
            {"show <path>", "lookup <path>"},
            {"--json", "--porcelain", "--fields"},
            {"status", "path", "provider", "available", "width", "height"},
            {"lofibox artwork lookup song.flac --json"},
            {"Artwork writeback follows metadata authority policy; raw provider URLs are not exposed as secrets."},
            {"If artwork is absent, verify embedded artwork and network policy with doctor."}},
        {"fingerprint", "direct", false, true,
            "Generate or match track identity using fingerprint and metadata seeds.",
            "lofibox fingerprint show|generate|match <path>",
            {"show <path>", "generate <path>", "match <path>"},
            {"--json", "--porcelain", "--fields"},
            {"status", "path", "provider", "found", "source", "confidence", "fingerprint", "recording_mbid", "release_mbid", "release_group_mbid", "title", "artist", "album", "audio_fingerprint_verified"},
            {"lofibox fingerprint generate song.flac --json", "lofibox fingerprint match song.flac"},
            {"Fingerprint/identity may contact online services when available; API keys are treated as credentials."},
            {"If fingerprinting is unavailable, install fpcalc/chromaprint and rerun doctor."}},
        {"cache", "direct", false, true,
            "Inspect and purge governed cache buckets.",
            "lofibox cache status|purge|clear|gc",
            {"status", "purge", "clear", "gc"},
            {"--json", "--porcelain", "--fields"},
            {"root", "entries", "bytes", "size", "removed_entries", "before_entries", "after_entries"},
            {"lofibox cache status --json", "lofibox cache gc"},
            {"purge/clear removes cached enrichment data and remote directory cache, not user music files."},
            {"If enrichment looks stale, purge cache and rerun metadata/lyrics lookup."}},
        {"now", "runtime-query", true, false,
            "Query the full live runtime snapshot from a running LoFiBox instance.",
            "lofibox now [--json] [--fields playback.status,playback.title,queue.index,...]",
            {},
            {"--runtime-socket <path>", "--json", "--porcelain", "--fields", "--quiet"},
            {"version", "playback.status", "playback.title", "playback.artist", "playback.elapsed", "playback.duration", "queue.index", "queue.visible", "eq.enabled", "remote.connection", "lyrics.available", "lyrics.status", "visualization.available"},
            {"lofibox now --json", "lofibox now --fields playback.status,playback.title,lyrics.available --json"},
            {"Read-only query; does not stop or start playback."},
            {"Exit 5 means the runtime socket is not reachable; start GUI/TUI/runtime or pass --runtime-socket."}},
        {"runtime playback", "runtime-query", true, false,
            "Query live playback status only.",
            "lofibox runtime playback [--json] [--fields status,title,artist,elapsed,duration]",
            {},
            {"--runtime-socket <path>", "--json", "--porcelain", "--fields", "--quiet"},
            {"status", "track", "title", "artist", "album", "source", "source_type", "elapsed", "duration", "live", "seekable", "audio", "volume", "codec", "bitrate", "shuffle", "repeat", "version"},
            {"lofibox runtime playback --fields status,title,artist --json"},
            {},
            {"If fields are UNKNOWN, verify playback enrichment logs and source facts."}},
        {"play", "runtime-command", true, true,
            "Mutate the live playback session or start a local/remote track.",
            "lofibox play [<path-or-url>|--id <track-id>|--source <profile-id> --item <remote-item-id>|--pause|--resume|--toggle|--next|--prev|--stop|--seek <seconds|MM:SS>|--shuffle on|off|--repeat off|one|all]",
            {"[path-or-url]"},
            {"--runtime-socket <path>", "--id <track-id>", "--source <profile-id>", "--item <remote-item-id>", "--pause", "--resume", "--toggle", "--next", "--prev", "--stop", "--seek <seconds|MM:SS>", "--shuffle on|off", "--repeat off|one|all", "--json", "--quiet"},
            {},
            {"lofibox play --id 12", "lofibox play --source jellyfin-home --item 30496", "lofibox play --shuffle off"},
            {"Mutates live playback only through the runtime command bus.", "q/exit in TUI does not imply stop."},
            {"Use --json errors to distinguish invalid arguments from runtime transport failure."}},
        {"queue", "runtime-command", true, true,
            "Query or mutate the active live queue.",
            "lofibox queue show|set <index>|jump <index>|next|clear",
            {"show", "set <index>", "jump <index>", "next", "clear"},
            {"--runtime-socket <path>", "--json", "--porcelain", "--fields", "--quiet"},
            {"index", "tracks", "visible", "shuffle", "repeat", "version"},
            {"lofibox queue show --json", "lofibox queue set 3", "lofibox queue clear"},
            {"clear mutates the live queue; it does not delete library files."},
            {"If jump fails on a remote item, verify the remote item is resolvable."}},
        {"shuffle", "runtime-command", true, true,
            "Idempotently set live shuffle state.",
            "lofibox shuffle on|off",
            {"on|off"},
            {"--runtime-socket <path>", "--json", "--quiet"},
            {},
            {"lofibox shuffle on", "lofibox shuffle off"},
            {"This is a set operation, not a toggle."},
            {"Query now --fields playback.shuffle --json after applying."}},
        {"eq", "runtime-command", true, true,
            "Query or mutate the live 10-band EQ runtime state.",
            "lofibox eq show|enable|disable|preset <name>|band <index> <gain>|reset",
            {"show", "enable", "disable", "preset <name>", "band <index> <gain>", "reset"},
            {"--runtime-socket <path>", "--json", "--porcelain", "--fields", "--quiet"},
            {"enabled", "preset", "bands", "version"},
            {"lofibox eq show --json", "lofibox eq preset Rock", "lofibox eq band 4 3"},
            {"EQ changes affect the live runtime DSP state."},
            {"Allowed built-in presets include Flat, Bass Boost, Treble Boost, Vocal, Rock, Pop, Jazz, Classical, Electronic, Podcast / Speech."}},
        {"tui", "presentation", true, false,
            "Launch the terminal-native runtime projection.",
            "lofibox tui [dashboard|now|lyrics|spectrum|queue|library|sources|eq|diagnostics|creator] [options]",
            {"[view]"},
            {"--runtime-socket <path>", "--charset unicode|ascii|minimal", "--theme amber|dark|light|mono", "--no-color", "--layout wide|normal|compact|micro|tiny", "--refresh <duration>"},
            {},
            {"lofibox tui", "lofibox tui lyrics --charset unicode", "lofibox-tui spectrum --no-color"},
            {"TUI is a projection and exits without stopping playback."},
            {"If disconnected, run lofibox runtime status or pass --runtime-socket."}},
    };
}

std::string commandPathFromArgv(int argc, char** argv, int start)
{
    std::string path{};
    for (int index = start; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--runtime-socket" || current == "--fields") {
            ++index;
            continue;
        }
        if (!current.empty() && current.front() == '-') {
            continue;
        }
        if (!path.empty()) {
            path.push_back(' ');
        }
        path.append(current);
    }
    return path;
}

std::vector<CommandSchema> matchingSchemas(std::string_view path)
{
    const auto schemas = commandSchemas();
    if (path.empty()) {
        return schemas;
    }
    std::vector<CommandSchema> matches{};
    for (const auto& schema : schemas) {
        if (schema.command == path) {
            matches.push_back(schema);
        }
    }
    if (!matches.empty()) {
        return matches;
    }
    for (const auto& schema : schemas) {
        if (schema.command == path || (schema.command.size() > path.size()
                && schema.command.substr(0, path.size()) == path
                && schema.command[path.size()] == ' ')
            || (path.size() > schema.command.size()
                && path.substr(0, schema.command.size()) == schema.command
                && path[schema.command.size()] == ' ')) {
            matches.push_back(schema);
        }
    }
    return matches;
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
        << "  commands --json  Discover command schemas for agents and automation.\n"
        << "  help [command]   Show global or command-family help.\n"
        << '\n'
        << "Direct commands:\n"
        << "  version\n"
        << "  source list|show|add|update|remove|probe|auth-status|capabilities\n"
        << "  credentials list|show-ref|set|status|validate|delete\n"
        << "  library scan|status|list|query\n"
        << "  search <text>\n"
        << "  remote browse|recent|search|resolve|stream-info\n"
        << "  metadata show|lookup|apply <path>\n"
        << "  lyrics show|lookup|apply <path>\n"
        << "  artwork show|lookup <path>\n"
        << "  fingerprint show|generate|match <path>\n"
        << "  cache status|purge|gc\n"
        << "  doctor [--json]\n"
        << '\n'
        << "Runtime commands (requires a running LoFiBox instance):\n"
        << "  now\n"
        << "  runtime status|playback|queue|eq|remote|settings\n"
        << "  play [--id <track-id>|--source <profile-id> --item <remote-item-id>|--pause|--resume|--toggle|--next|--prev|--stop|--seek <seconds>|--shuffle on|off|--repeat off|one|all]\n"
        << "  pause|resume|toggle|stop|seek <seconds>|next|prev\n"
        << "  queue show|set <index>|next|clear\n"
        << "  shuffle on|off|repeat off|all|one\n"
        << "  eq show|enable|disable|preset <name>|band <index> <gain>\n"
        << "  remote reconnect|runtime reload|runtime shutdown\n"
        << '\n'
        << "Terminal UI:\n"
        << "  lofibox tui [dashboard|now|lyrics|spectrum|queue|library|sources|eq|diagnostics|creator]\n"
        << "  lofibox-tui [view] [--charset unicode|ascii|minimal] [--theme amber|dark|light|mono]\n"
        << '\n'
        << "Environment:\n"
        << "  LOFIBOX_ASSET_DIR   Override the runtime asset directory.\n"
        << "  LOFIBOX_FONT_PATH   Override the system font file used for text rendering.\n";
}

void printSchemaObject(std::ostream& out, const CommandSchema& schema)
{
    out << '{';
    bool field_first = true;
    cli::printJsonField(out, "command", schema.command, field_first);
    cli::printJsonField(out, "kind", schema.kind, field_first);
    cli::printJsonBoolField(out, "requires_runtime", schema.requires_runtime, field_first);
    cli::printJsonBoolField(out, "mutates", schema.mutates, field_first);
    cli::printJsonField(out, "summary", schema.summary, field_first);
    cli::printJsonField(out, "usage", schema.usage, field_first);
    printJsonStringArray(out, "positionals", schema.positionals, field_first);
    printJsonStringArray(out, "options", schema.options, field_first);
    printJsonStringArray(out, "fields", schema.fields, field_first);
    printJsonStringArray(out, "examples", schema.examples, field_first);
    printJsonStringArray(out, "safety", schema.safety, field_first);
    printJsonStringArray(out, "recovery", schema.recovery, field_first);
    if (!field_first) {
        out << ',';
    }
    cli::printJsonString(out, "exit_codes");
    out << ":{"
        << "\"0\":\"success\","
        << "\"2\":\"usage or invalid argument\","
        << "\"3\":\"resource not found\","
        << "\"4\":\"credential missing or authentication failed\","
        << "\"5\":\"network or runtime transport unavailable\","
        << "\"6\":\"playback backend failure\","
        << "\"7\":\"persistence failure\""
        << "}}";
}

void printSchemaListJson(std::ostream& out, const std::vector<CommandSchema>& schemas)
{
    out << "{\"name\":\"lofibox\",\"version\":\"" << kVersion << "\",\"commands\":[";
    bool first = true;
    for (const auto& schema : schemas) {
        if (!first) {
            out << ',';
        }
        first = false;
        printSchemaObject(out, schema);
    }
    out << "]}\n";
}

void printSchemaResultJson(std::ostream& out, const std::vector<CommandSchema>& schemas)
{
    if (schemas.size() == 1U) {
        printSchemaObject(out, schemas.front());
        out << '\n';
        return;
    }
    printSchemaListJson(out, schemas);
}

void printTextSchemaHelp(std::ostream& out, const std::vector<CommandSchema>& schemas)
{
    if (schemas.empty()) {
        printHelp(out);
        return;
    }
    if (schemas.size() > 1U) {
        out << "Matching LoFiBox commands:\n";
        for (const auto& schema : schemas) {
            out << "  " << schema.command << "  - " << schema.summary << '\n';
        }
        out << "\nUse: lofibox help <command> --json\n";
        return;
    }
    const auto& schema = schemas.front();
    out << "LoFiBox command: " << schema.command << '\n'
        << schema.summary << "\n\n"
        << "Usage:\n"
        << "  " << schema.usage << "\n\n"
        << "Kind: " << schema.kind << '\n'
        << "Requires runtime: " << (schema.requires_runtime ? "yes" : "no") << '\n'
        << "Mutates state: " << (schema.mutates ? "yes" : "no") << "\n\n";
    const auto printList = [&out](std::string_view title, const std::vector<std::string_view>& values) {
        if (values.empty()) {
            return;
        }
        out << title << ":\n";
        for (const auto value : values) {
            out << "  " << value << '\n';
        }
        out << '\n';
    };
    printList("Positionals", schema.positionals);
    printList("Options", schema.options);
    printList("Fields", schema.fields);
    printList("Examples", schema.examples);
    printList("Safety", schema.safety);
    printList("Recovery", schema.recovery);
    out << "Exit codes:\n"
        << "  0 success\n"
        << "  2 usage or invalid argument\n"
        << "  3 resource not found\n"
        << "  4 credential missing or authentication failed\n"
        << "  5 network or runtime transport unavailable\n"
        << "  6 playback backend failure\n"
        << "  7 persistence failure\n";
}

} // namespace

bool handleCommonCliOptions(int argc, char** argv, std::ostream& out)
{
    const auto command = firstCommand(argc, argv);
    if (hasOption(argc, argv, "--schema")) {
        const auto path = commandPathFromArgv(argc, argv, 1);
        const auto matches = matchingSchemas(path);
        if (hasOption(argc, argv, "--json") || matches.size() == 1U) {
            printSchemaResultJson(out, matches.empty() ? commandSchemas() : matches);
        } else {
            printTextSchemaHelp(out, matches);
        }
        return true;
    }
    if (command == "version") {
        printVersion(out, argc, argv);
        return true;
    }
    if (command == "commands") {
        if (hasOption(argc, argv, "--json")) {
            printSchemaListJson(out, commandSchemas());
        } else {
            printHelp(out);
        }
        return true;
    }
    if (command == "help") {
        const auto requested_path = commandPathFromArgv(argc, argv, 2);
        const auto matches = matchingSchemas(requested_path);
        if (hasOption(argc, argv, "--json")) {
            printSchemaResultJson(out, matches.empty() ? commandSchemas() : matches);
        } else {
            if (requested_path.empty() || matches.empty()) {
                printHelp(out);
            } else {
                printTextSchemaHelp(out, matches);
            }
        }
        return true;
    }
    if (hasOption(argc, argv, "--help") || hasOption(argc, argv, "-h")) {
        if (command.empty()) {
            printHelp(out);
            return true;
        }
        if (command != "tui") {
            const auto path = commandPathFromArgv(argc, argv, 1);
            const auto matches = matchingSchemas(path);
            if (!matches.empty()) {
                printTextSchemaHelp(out, matches);
                return true;
            }
        }
    }

    for (int index = 1; index < argc; ++index) {
        const std::string_view current{argv[index]};
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
        if (current == "help" || current == "commands" || current == "--help" || current == "-h" || current == "--version") {
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
