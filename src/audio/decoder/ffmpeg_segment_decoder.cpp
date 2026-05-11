// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/decoder/ffmpeg_segment_decoder.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
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

namespace fs = std::filesystem;

namespace lofibox::audio::decoder {
namespace {

std::atomic_uint64_t g_decode_counter{0};

[[nodiscard]] std::string sourceInputFromUri(std::string_view uri)
{
    constexpr std::string_view kFilePrefix{"file://"};
    if (uri.starts_with(kFilePrefix)) {
        return std::string{uri.substr(kFilePrefix.size())};
    }
    return std::string{uri};
}

[[nodiscard]] std::string secondsArgument(double seconds)
{
    std::ostringstream stream{};
    stream << std::fixed << std::setprecision(6) << std::max(0.0, seconds);
    return stream.str();
}

[[nodiscard]] fs::path temporaryRawPath()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto counter = g_decode_counter.fetch_add(1, std::memory_order_relaxed);
    return fs::temp_directory_path()
        / ("lofibox-groove-segment-" + std::to_string(now) + "-" + std::to_string(counter) + ".f32le");
}

#if defined(_WIN32)
[[nodiscard]] std::wstring readEnvWide(const wchar_t* name)
{
    if (name == nullptr) {
        return {};
    }
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

[[nodiscard]] std::optional<fs::path> resolveFfmpegExecutable()
{
    if (const auto env_path = readEnvWide(L"FFMPEG_PATH"); !env_path.empty() && fs::exists(env_path)) {
        return fs::path(env_path);
    }

    wchar_t resolved[MAX_PATH]{};
    if (SearchPathW(nullptr, L"ffmpeg.exe", nullptr, MAX_PATH, resolved, nullptr) > 0) {
        return fs::path(resolved);
    }

    const auto local_app_data = readEnvWide(L"LOCALAPPDATA");
    if (!local_app_data.empty()) {
        const fs::path winget_link = fs::path(local_app_data) / "Microsoft" / "WinGet" / "Links" / "ffmpeg.exe";
        if (fs::exists(winget_link)) {
            return winget_link;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::wstring utf8ToWide(std::string_view text)
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

[[nodiscard]] std::wstring quoteWindowsArg(std::string_view arg)
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

[[nodiscard]] std::wstring quoteWindowsPath(const fs::path& path)
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

[[nodiscard]] std::wstring buildWindowsCommandLine(const fs::path& executable, const std::vector<std::string>& args)
{
    std::wstring command = quoteWindowsPath(executable);
    for (const auto& arg : args) {
        command += L" ";
        command += quoteWindowsArg(arg);
    }
    return command;
}

[[nodiscard]] bool runFfmpeg(const fs::path& executable, const std::vector<std::string>& args)
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

    const DWORD wait_result = WaitForSingleObject(process.hProcess, 60000);
    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 1000);
    }
    DWORD exit_code = 1;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return wait_result != WAIT_TIMEOUT && exit_code == 0;
}
#elif defined(__linux__)
[[nodiscard]] std::optional<fs::path> resolveFfmpegExecutable()
{
    if (const char* env_path = std::getenv("FFMPEG_PATH"); env_path != nullptr && *env_path != '\0' && fs::exists(env_path)) {
        return fs::path(env_path);
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
            candidate /= "ffmpeg";
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

[[nodiscard]] bool runFfmpeg(const fs::path& executable, const std::vector<std::string>& args)
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
#else
[[nodiscard]] std::optional<fs::path> resolveFfmpegExecutable()
{
    return std::nullopt;
}

[[nodiscard]] bool runFfmpeg(const fs::path&, const std::vector<std::string>&)
{
    return false;
}
#endif

[[nodiscard]] std::optional<std::vector<float>> readF32LeFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>{});
    if (bytes.size() < sizeof(float)) {
        return std::nullopt;
    }
    bytes.resize(bytes.size() - (bytes.size() % sizeof(float)));

    std::vector<float> samples{};
    samples.reserve(bytes.size() / sizeof(float));
    for (std::size_t offset = 0; offset + sizeof(float) <= bytes.size(); offset += sizeof(float)) {
        const std::uint32_t bits =
            static_cast<std::uint32_t>(bytes[offset])
            | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U)
            | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U)
            | (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
        float sample = 0.0f;
        std::memcpy(&sample, &bits, sizeof(sample));
        samples.push_back(sample);
    }
    return samples;
}

} // namespace

std::optional<DecodedAudioChunk> FfmpegSegmentDecoder::decodeSegment(
    std::string_view source_uri,
    double start_seconds,
    double duration_seconds,
    int output_sample_rate_hz,
    int output_channels) const
{
    lastError_.clear();
    if (source_uri.empty()) {
        lastError_ = "CAPTURE SOURCE EMPTY";
        return std::nullopt;
    }
    if (duration_seconds <= 0.0 || output_sample_rate_hz <= 0 || output_channels <= 0) {
        lastError_ = "CAPTURE RANGE INVALID";
        return std::nullopt;
    }

    const auto executable = resolveFfmpegExecutable();
    if (!executable.has_value()) {
        lastError_ = "CAPTURE DECODER UNAVAILABLE";
        return std::nullopt;
    }

    const auto raw_path = temporaryRawPath();
    std::vector<std::string> args{
        "-hide_banner",
        "-loglevel",
        "error",
        "-nostdin",
        "-y",
        "-i",
        sourceInputFromUri(source_uri),
        "-ss",
        secondsArgument(start_seconds),
        "-t",
        secondsArgument(duration_seconds),
        "-map",
        "0:a:0",
        "-vn",
        "-ac",
        std::to_string(output_channels),
        "-ar",
        std::to_string(output_sample_rate_hz),
        "-f",
        "f32le",
        raw_path.string(),
    };

    std::error_code ec{};
    const bool ok = runFfmpeg(*executable, args);
    if (!ok || !fs::exists(raw_path, ec)) {
        fs::remove(raw_path, ec);
        lastError_ = "CAPTURE DECODE FAILED";
        return std::nullopt;
    }

    auto samples = readF32LeFile(raw_path);
    fs::remove(raw_path, ec);
    if (!samples.has_value() || samples->empty()) {
        lastError_ = "CAPTURE DECODE EMPTY";
        return std::nullopt;
    }

    const auto frame_samples = static_cast<std::size_t>(std::max(1, output_channels));
    samples->resize(samples->size() - (samples->size() % frame_samples));
    if (samples->empty()) {
        lastError_ = "CAPTURE DECODE EMPTY";
        return std::nullopt;
    }

    DecodedAudioChunk chunk{};
    chunk.interleaved_samples = std::move(*samples);
    chunk.sample_rate_hz = output_sample_rate_hz;
    chunk.channels = output_channels;
    chunk.frame_index = static_cast<std::uint64_t>(std::max(0.0, start_seconds) * static_cast<double>(output_sample_rate_hz));
    return chunk;
}

std::string FfmpegSegmentDecoder::lastErrorMessage() const
{
    return lastError_;
}

} // namespace lofibox::audio::decoder
