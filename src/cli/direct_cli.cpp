// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli/direct_cli.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/remote_profile_store.h"
#include "application/app_service_host.h"
#include "application/app_service_registry.h"
#include "application/credential_command_service.h"
#include "application/source_profile_command_service.h"

namespace lofibox::cli {
namespace {

struct CliOption {
    std::string name{};
    std::string value{};
};

struct ParsedArgs {
    std::vector<std::string_view> positional{};
    std::vector<CliOption> options{};
};

bool isDirectCommand(std::string_view value) noexcept
{
    return value == "source"
        || value == "library"
        || value == "doctor"
        || value == "cache"
        || value == "credentials";
}

ParsedArgs parseArgs(int argc, char** argv)
{
    ParsedArgs result{};
    for (int index = 1; index < argc; ++index) {
        std::string_view current{argv[index]};
        if (current.rfind("--", 0) == 0) {
            const auto eq = current.find('=');
            if (eq != std::string_view::npos) {
                result.options.push_back({std::string{current.substr(2, eq - 2)}, std::string{current.substr(eq + 1)}});
            } else if ((index + 1) < argc && std::string_view{argv[index + 1]}.rfind("-", 0) != 0) {
                result.options.push_back({std::string{current.substr(2)}, argv[index + 1]});
                ++index;
            } else {
                result.options.push_back({std::string{current.substr(2)}, "true"});
            }
            continue;
        }
        result.positional.push_back(current);
    }
    return result;
}

std::optional<std::string> optionValue(const ParsedArgs& args, std::string_view name)
{
    for (const auto& option : args.options) {
        if (option.name == name) {
            return option.value;
        }
    }
    return std::nullopt;
}

bool optionPresent(const ParsedArgs& args, std::string_view name)
{
    return optionValue(args, name).has_value();
}

std::optional<std::size_t> findProfileById(const std::vector<app::RemoteServerProfile>& profiles, std::string_view id)
{
    const auto found = std::find_if(profiles.begin(), profiles.end(), [id](const app::RemoteServerProfile& profile) {
        return profile.id == id;
    });
    if (found == profiles.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(profiles.begin(), found));
}

std::string stateLabel(app::LibraryIndexState state)
{
    switch (state) {
    case app::LibraryIndexState::Uninitialized: return "UNINITIALIZED";
    case app::LibraryIndexState::Loading: return "LOADING";
    case app::LibraryIndexState::Ready: return "READY";
    case app::LibraryIndexState::Degraded: return "DEGRADED";
    }
    return "UNKNOWN";
}

std::string bytesLabel(std::uintmax_t bytes)
{
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        return std::to_string(bytes / (1024ULL * 1024ULL * 1024ULL)) + "G";
    }
    if (bytes >= 1024ULL * 1024ULL) {
        return std::to_string(bytes / (1024ULL * 1024ULL)) + "M";
    }
    if (bytes >= 1024ULL) {
        return std::to_string(bytes / 1024ULL) + "K";
    }
    return std::to_string(bytes) + "B";
}

std::vector<std::filesystem::path> rootsFromOption(const ParsedArgs& args)
{
    std::vector<std::filesystem::path> roots{};
    if (const auto root = optionValue(args, "root")) {
        roots.emplace_back(*root);
    }
    return roots;
}

void printDirectHelp(std::ostream& out)
{
    out << "Direct LoFiBox commands:\n"
        << "  lofibox source list\n"
        << "  lofibox source add <kind> --id <id> --name <label> --url <url> [--user <user>] [--password <secret>] [--token <secret>]\n"
        << "  lofibox source update <id> [--name <label>] [--url <url>] [--user <user>] [--password <secret>] [--token <secret>]\n"
        << "  lofibox source probe <id>\n"
        << "  lofibox credentials set <source-id> [--user <user>] [--password <secret>] [--token <secret>]\n"
        << "  lofibox credentials status <source-id>\n"
        << "  lofibox credentials delete <source-id>\n"
        << "  lofibox library scan [path...]\n"
        << "  lofibox library status [--root <path>]\n"
        << "  lofibox library query <text> [--root <path>]\n"
        << "  lofibox cache status\n"
        << "  lofibox cache clear\n"
        << "  lofibox doctor\n";
}

void applyProfileOptions(
    const ParsedArgs& args,
    application::SourceProfileCommandService source_profiles,
    app::RemoteServerProfile& profile,
    std::size_t profile_count)
{
    if (const auto value = optionValue(args, "name")) {
        (void)source_profiles.updateTextField(profile, application::SourceProfileTextField::Label, *value, profile_count);
    }
    if (const auto value = optionValue(args, "url")) {
        (void)source_profiles.updateTextField(profile, application::SourceProfileTextField::Address, *value, profile_count);
    }
    if (const auto value = optionValue(args, "user")) {
        (void)source_profiles.updateTextField(profile, application::SourceProfileTextField::Username, *value, profile_count);
    }
    if (optionPresent(args, "insecure")) {
        profile.tls_policy.verify_peer = false;
    }
    if (optionPresent(args, "allow-self-signed")) {
        profile.tls_policy.allow_self_signed = true;
        profile.tls_policy.verify_peer = false;
    }
}

application::CredentialSecretPatch credentialPatchFromOptions(const ParsedArgs& args)
{
    application::CredentialSecretPatch patch{};
    if (const auto value = optionValue(args, "user")) {
        patch.username = *value;
    }
    if (const auto value = optionValue(args, "password")) {
        patch.password = *value;
    }
    if (const auto value = optionValue(args, "token")) {
        patch.api_token = *value;
    }
    return patch;
}

bool hasCredentialPatch(const application::CredentialSecretPatch& patch) noexcept
{
    return !patch.username.empty() || !patch.password.empty() || !patch.api_token.empty();
}

int runSourceCommand(const ParsedArgs& args, application::AppServiceRegistry& registry, std::ostream& out, std::ostream& err)
{
    if (args.positional.size() < 2U || args.positional[1] == "help") {
        printDirectHelp(out);
        return 0;
    }

    auto profiles = registry.sourceProfiles().loadProfiles();
    const auto verb = args.positional[1];
    if (verb == "list") {
        for (const auto& profile : profiles) {
            out << profile.id << '\t'
                << app::remoteServerKindToString(profile.kind) << '\t'
                << registry.sourceProfiles().profileLabel(profile) << '\t'
                << (profile.base_url.empty() ? "-" : profile.base_url) << '\t'
                << registry.sourceProfiles().readiness(profile) << '\n';
        }
        return 0;
    }

    if (verb == "add") {
        if (args.positional.size() < 3U) {
            err << "source add requires a kind.\n";
            return 2;
        }
        auto profile = registry.sourceProfiles().createProfile(app::remoteServerKindFromString(std::string{args.positional[2]}), profiles.size());
        if (const auto id = optionValue(args, "id")) {
            profile.id = *id;
            profile.credential_ref.id.clear();
            registry.sourceProfiles().ensureCredentialRef(profile, profiles.size() + 1U);
        }
        applyProfileOptions(args, registry.sourceProfiles(), profile, profiles.size() + 1U);
        auto patch = credentialPatchFromOptions(args);
        if (hasCredentialPatch(patch) && registry.sourceProfiles().supportsCredentials(profile.kind)) {
            const auto credential_result = registry.credentials().setSecret(profile, patch);
            if (!credential_result.accepted) {
                err << credential_result.summary << '\n';
                return 1;
            }
        }
        profiles.push_back(std::move(profile));
        if (!registry.sourceProfiles().persistProfiles(profiles)) {
            err << "failed to save source profiles.\n";
            return 1;
        }
        out << "ADDED " << profiles.back().id << '\n';
        return 0;
    }

    if (verb == "update") {
        if (args.positional.size() < 3U) {
            err << "source update requires a source id.\n";
            return 2;
        }
        const auto index = findProfileById(profiles, args.positional[2]);
        if (!index) {
            err << "source not found: " << args.positional[2] << '\n';
            return 1;
        }
        auto& profile = profiles[*index];
        applyProfileOptions(args, registry.sourceProfiles(), profile, profiles.size());
        auto patch = credentialPatchFromOptions(args);
        if (hasCredentialPatch(patch) && registry.sourceProfiles().supportsCredentials(profile.kind)) {
            const auto credential_result = registry.credentials().setSecret(profile, patch);
            if (!credential_result.accepted) {
                err << credential_result.summary << '\n';
                return 1;
            }
        }
        if (!registry.sourceProfiles().persistProfiles(profiles)) {
            err << "failed to save source profiles.\n";
            return 1;
        }
        out << "UPDATED " << profile.id << '\n';
        return 0;
    }

    if (verb == "probe") {
        if (args.positional.size() < 3U) {
            err << "source probe requires a source id.\n";
            return 2;
        }
        const auto index = findProfileById(profiles, args.positional[2]);
        if (!index) {
            err << "source not found: " << args.positional[2] << '\n';
            return 1;
        }
        const auto probe = registry.sourceProfiles().probe(profiles[*index], profiles.size());
        out << profiles[*index].id << '\t'
            << (probe.session.available ? "ONLINE" : "OFFLINE") << '\t'
            << (probe.session.message.empty() ? probe.command.summary : probe.session.message) << '\n';
        return probe.session.available ? 0 : 1;
    }

    err << "unknown source command: " << verb << '\n';
    return 2;
}

int runCredentialsCommand(const ParsedArgs& args, application::AppServiceRegistry& registry, std::ostream& out, std::ostream& err)
{
    if (args.positional.size() < 3U || args.positional[1] == "help") {
        printDirectHelp(out);
        return 0;
    }
    auto profiles = registry.sourceProfiles().loadProfiles();
    const auto index = findProfileById(profiles, args.positional[2]);
    if (!index) {
        err << "source not found: " << args.positional[2] << '\n';
        return 1;
    }
    auto& profile = profiles[*index];
    const auto verb = args.positional[1];
    if (verb == "status") {
        const auto status = registry.credentials().status(profile);
        out << profile.id << '\t'
            << (status.attached ? status.ref_id : "NONE") << '\t'
            << "USER=" << (status.has_username ? "SET" : "MISSING") << '\t'
            << "PASSWORD=" << (status.has_password ? "SET" : "MISSING") << '\t'
            << "TOKEN=" << (status.has_api_token ? "SET" : "MISSING") << '\n';
        return 0;
    }
    if (verb == "set") {
        auto patch = credentialPatchFromOptions(args);
        if (!hasCredentialPatch(patch)) {
            err << "credentials set requires --user, --password, or --token.\n";
            return 2;
        }
        const auto result = registry.credentials().setSecret(profile, patch);
        if (!result.accepted) {
            err << result.summary << '\n';
            return 1;
        }
        (void)registry.sourceProfiles().persistProfiles(profiles);
        out << "SAVED " << profile.credential_ref.id << '\n';
        return 0;
    }
    if (verb == "delete") {
        const auto result = registry.credentials().deleteSecret(profile);
        if (!result.accepted) {
            err << result.summary << '\n';
            return 1;
        }
        (void)registry.sourceProfiles().persistProfiles(profiles);
        out << "DELETED " << profile.id << '\n';
        return 0;
    }

    err << "unknown credentials command: " << verb << '\n';
    return 2;
}

int runLibraryCommand(
    const ParsedArgs& args,
    application::AppServiceRegistry& registry,
    std::ostream& out,
    std::ostream& err)
{
    if (args.positional.size() < 2U || args.positional[1] == "help") {
        printDirectHelp(out);
        return 0;
    }
    const auto verb = args.positional[1];
    if (verb == "scan") {
        std::vector<std::filesystem::path> roots{};
        for (std::size_t index = 2; index < args.positional.size(); ++index) {
            roots.emplace_back(std::string{args.positional[index]});
        }
        if (!registry.libraryMutations().refreshLibrary(roots)) {
            err << "library scan unavailable.\n";
            return 1;
        }
        const auto& model = registry.libraryQueries().model();
        out << "STATE\t" << stateLabel(registry.libraryQueries().state()) << '\n'
            << "TRACKS\t" << model.tracks.size() << '\n'
            << "ALBUMS\t" << model.albums.size() << '\n'
            << "ARTISTS\t" << model.artists.size() << '\n';
        return registry.libraryQueries().state() == app::LibraryIndexState::Degraded ? 1 : 0;
    }

    if (verb == "status") {
        if (!registry.libraryMutations().refreshLibrary(rootsFromOption(args))) {
            err << "library scan unavailable.\n";
            return 1;
        }
        const auto& model = registry.libraryQueries().model();
        out << "STATE\t" << stateLabel(registry.libraryQueries().state()) << '\n'
            << "TRACKS\t" << model.tracks.size() << '\n'
            << "ALBUMS\t" << model.albums.size() << '\n'
            << "ARTISTS\t" << model.artists.size() << '\n'
            << "GENRES\t" << model.genres.size() << '\n';
        return registry.libraryQueries().state() == app::LibraryIndexState::Degraded ? 1 : 0;
    }

    if (verb == "query") {
        if (args.positional.size() < 3U) {
            err << "library query requires text.\n";
            return 2;
        }
        std::string query{};
        for (std::size_t index = 2; index < args.positional.size(); ++index) {
            if (!query.empty()) {
                query.push_back(' ');
            }
            query.append(args.positional[index]);
        }
        if (!registry.libraryMutations().refreshLibrary(rootsFromOption(args))) {
            err << "library scan unavailable.\n";
            return 1;
        }
        const auto items = registry.libraryQueries().searchLocal(query, 20);
        for (const auto& item : items) {
            out << item.id << '\t' << item.title << '\t' << item.artist << '\t' << item.album << '\n';
        }
        if (items.empty()) {
            out << "NO RESULTS\n";
        }
        return 0;
    }

    err << "unknown library command: " << verb << '\n';
    return 2;
}

int runCacheCommand(const ParsedArgs& args, application::AppServiceRegistry& registry, std::ostream& out, std::ostream& err)
{
    if (args.positional.size() < 2U || args.positional[1] == "help") {
        printDirectHelp(out);
        return 0;
    }
    if (args.positional[1] == "status") {
        const auto result = registry.cacheCommands().status();
        if (!result.command.accepted) {
            err << result.command.summary << '\n';
            return 1;
        }
        out << "ROOT\t" << result.root.string() << '\n'
            << "ENTRIES\t" << result.usage.entry_count << '\n'
            << "BYTES\t" << result.usage.total_bytes << '\n'
            << "SIZE\t" << bytesLabel(result.usage.total_bytes) << '\n';
        return 0;
    }
    if (args.positional[1] == "clear") {
        const auto result = registry.cacheCommands().clearAll();
        if (!result.command.accepted) {
            err << result.command.summary << '\n';
            return 1;
        }
        out << "CLEARED\t" << result.removed_entries << '\n';
        return 0;
    }
    err << "unknown cache command: " << args.positional[1] << '\n';
    return 2;
}

int runDoctorCommand(application::AppServiceRegistry& registry, std::ostream& out)
{
    const auto profiles = registry.sourceProfiles().loadProfiles();
    const auto snapshot = registry.diagnostics().snapshot(profiles);
    out << "STATUS\t" << snapshot.command.summary << '\n';
    for (const auto& capability : snapshot.capabilities) {
        out << "CAPABILITY\t" << capability.name << '\t'
            << (capability.available ? "OK" : "MISSING") << '\t'
            << capability.detail << '\n';
    }
    for (const auto& source : snapshot.sources) {
        out << "SOURCE\t" << source.source_label << '\t'
            << source.connection_status << '\t'
            << source.permission << '\n';
    }
    return snapshot.degraded ? 1 : 0;
}

} // namespace

std::optional<int> runDirectCliCommand(
    int argc,
    char** argv,
    app::RuntimeServices& services,
    std::ostream& out,
    std::ostream& err)
{
    if (argc < 2) {
        return std::nullopt;
    }
    const std::string_view command{argv[1]};
    if (!isDirectCommand(command)) {
        return std::nullopt;
    }

    auto args = parseArgs(argc, argv);
    application::AppServiceHost service_host{services};
    auto registry = service_host.registry();

    if (command == "source") {
        return runSourceCommand(args, registry, out, err);
    }
    if (command == "credentials") {
        return runCredentialsCommand(args, registry, out, err);
    }
    if (command == "cache") {
        return runCacheCommand(args, registry, out, err);
    }
    if (command == "doctor") {
        return runDoctorCommand(registry, out);
    }
    if (command == "library") {
        return runLibraryCommand(args, registry, out, err);
    }
    return std::nullopt;
}

} // namespace lofibox::cli
