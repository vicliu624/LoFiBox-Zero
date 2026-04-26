// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

#include "platform/host/runtime_paths.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {

std::mutex& logMutex()
{
    static std::mutex mutex{};
    return mutex;
}

fs::path logPath()
{
    return runtime_paths::runtimeLogPath();
}

std::string levelName(RuntimeLogLevel level)
{
    switch (level) {
    case RuntimeLogLevel::Debug: return "DEBUG";
    case RuntimeLogLevel::Info: return "INFO";
    case RuntimeLogLevel::Warn: return "WARN";
    case RuntimeLogLevel::Error: return "ERROR";
    }
    return "INFO";
}

} // namespace

void logRuntime(RuntimeLogLevel level, std::string_view category, std::string_view message)
{
    std::lock_guard<std::mutex> guard(logMutex());

    const fs::path path = logPath();
    std::error_code ec{};
    fs::create_directories(path.parent_path(), ec);

    std::ofstream output(path, std::ios::app);
    if (!output.is_open()) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    output << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
           << " [" << levelName(level) << "]"
           << " [" << category << "] "
           << message << '\n';
}

} // namespace lofibox::platform::host
