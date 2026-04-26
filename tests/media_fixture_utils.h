// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace test_media_fixture {
namespace fs = std::filesystem;

#if defined(_WIN32)
inline std::wstring utf8ToWide(std::string_view text)
{
    if (text.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), required);
    return result;
}

inline std::wstring quoteWindowsArg(std::string_view arg)
{
    std::wstring wide = utf8ToWide(arg);
    std::wstring quoted = L"\"";
    for (const wchar_t ch : wide) {
        if (ch == L'"') {
            quoted += L"\\\"";
        } else {
            quoted += ch;
        }
    }
    quoted += L"\"";
    return quoted;
}

inline std::wstring quoteWindowsPath(const fs::path& path)
{
    std::wstring quoted = L"\"";
    for (const wchar_t ch : path.wstring()) {
        if (ch == L'"') {
            quoted += L"\\\"";
        } else {
            quoted += ch;
        }
    }
    quoted += L"\"";
    return quoted;
}

inline std::wstring buildWindowsCommandLine(const fs::path& executable, const std::vector<std::string>& args)
{
    std::wstring command = quoteWindowsPath(executable);
    for (const auto& arg : args) {
        command += L" ";
        command += quoteWindowsArg(arg);
    }
    return command;
}

inline std::wstring readEnvWide(const wchar_t* name)
{
    const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) {
        return {};
    }

    std::wstring value(static_cast<std::size_t>(required), L'\0');
    const DWORD written = GetEnvironmentVariableW(name, value.data(), required);
    if (written == 0) {
        return {};
    }
    value.resize(static_cast<std::size_t>(written));
    return value;
}

inline std::optional<fs::path> resolveExecutablePath(const wchar_t* env_name, const wchar_t* executable_name)
{
    if (const auto env_path = readEnvWide(env_name); !env_path.empty() && fs::exists(env_path)) {
        return fs::path(env_path);
    }

    wchar_t resolved[MAX_PATH]{};
    if (SearchPathW(nullptr, executable_name, nullptr, MAX_PATH, resolved, nullptr) > 0) {
        return fs::path(resolved);
    }

    const auto local_app_data = readEnvWide(L"LOCALAPPDATA");
    if (!local_app_data.empty()) {
        const fs::path winget_link = fs::path(local_app_data) / "Microsoft" / "WinGet" / "Links" / executable_name;
        if (fs::exists(winget_link)) {
            return winget_link;
        }
    }

    return std::nullopt;
}

inline bool runCommand(const fs::path& executable, const std::vector<std::string>& args)
{
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::wstring command = buildWindowsCommandLine(executable, args);
    const BOOL created = CreateProcessW(
        executable.wstring().c_str(),
        command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startup,
        &process);
    if (!created) {
        return false;
    }

    WaitForSingleObject(process.hProcess, 10000);
    DWORD exit_code = 1;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exit_code == 0;
}
#elif defined(__linux__)
inline std::optional<fs::path> resolveExecutablePath(const char* env_name, const char* executable_name)
{
    if (env_name != nullptr) {
        if (const char* env_path = std::getenv(env_name); env_path != nullptr && *env_path != '\0' && fs::exists(env_path)) {
            return fs::path(env_path);
        }
    }

    const char* path_env = std::getenv("PATH");
    if (path_env == nullptr) {
        return std::nullopt;
    }

    std::string paths(path_env);
    std::size_t start = 0;
    while (start <= paths.size()) {
        const std::size_t end = paths.find(':', start);
        const std::string_view segment = end == std::string::npos
            ? std::string_view(paths).substr(start)
            : std::string_view(paths).substr(start, end - start);
        if (!segment.empty()) {
            fs::path candidate{std::string(segment)};
            candidate /= executable_name;
            std::error_code ec{};
            if (fs::exists(candidate, ec) && !ec) {
                return candidate;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return std::nullopt;
}

inline bool runCommand(const fs::path& executable, const std::vector<std::string>& args)
{
    const pid_t child = fork();
    if (child < 0) {
        return false;
    }

    if (child == 0) {
        std::vector<std::string> owned_args{};
        owned_args.reserve(args.size() + 1);
        owned_args.push_back(executable.filename().string());
        owned_args.insert(owned_args.end(), args.begin(), args.end());

        std::vector<char*> argv{};
        argv.reserve(owned_args.size() + 1);
        for (auto& arg : owned_args) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        execv(executable.string().c_str(), argv.data());
        _exit(127);
    }

    int status = 0;
    waitpid(child, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif

} // namespace test_media_fixture
