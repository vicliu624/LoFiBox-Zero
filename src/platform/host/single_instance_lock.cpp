// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/single_instance_lock.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "platform/host/runtime_paths.h"

namespace lofibox::platform::host {
namespace {

std::string sanitizedName(std::string_view value)
{
    std::string result{};
    for (const char ch : value) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_') {
            result.push_back(ch);
        } else {
            result.push_back('-');
        }
    }
    return result.empty() ? "lofibox-zero" : result;
}

#if defined(_WIN32)
std::wstring widenAscii(std::string_view value)
{
    std::wstring result{};
    result.reserve(value.size());
    for (const unsigned char ch : value) {
        result.push_back(static_cast<wchar_t>(ch));
    }
    return result;
}
#elif defined(__linux__)
std::filesystem::path lockPathFor(std::string_view instance_name)
{
    if (const char* explicit_path = std::getenv("LOFIBOX_SINGLE_INSTANCE_LOCK")) {
        if (*explicit_path != '\0') {
            return explicit_path;
        }
    }

    return runtime_paths::appRuntimeDir() / (sanitizedName(instance_name) + "-" + std::to_string(static_cast<long long>(getuid())) + ".lock");
}
#endif

} // namespace

struct SingleInstanceLock::Impl {
    bool acquired{false};
    std::string message{};
#if defined(_WIN32)
    HANDLE mutex{nullptr};
#elif defined(__linux__)
    int fd{-1};
    std::filesystem::path path{};
#endif

    ~Impl()
    {
#if defined(_WIN32)
        if (mutex != nullptr) {
            if (acquired) {
                ReleaseMutex(mutex);
            }
            CloseHandle(mutex);
        }
#elif defined(__linux__)
        if (fd >= 0) {
            if (acquired) {
                flock(fd, LOCK_UN);
            }
            close(fd);
        }
#endif
    }
};

SingleInstanceLock::SingleInstanceLock() = default;
SingleInstanceLock::~SingleInstanceLock() = default;
SingleInstanceLock::SingleInstanceLock(SingleInstanceLock&&) noexcept = default;
SingleInstanceLock& SingleInstanceLock::operator=(SingleInstanceLock&&) noexcept = default;

SingleInstanceLock::SingleInstanceLock(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

SingleInstanceLock SingleInstanceLock::acquire(std::string_view instance_name)
{
    auto impl = std::make_unique<Impl>();
#if defined(_WIN32)
    const auto mutex_name = widenAscii("Local\\" + sanitizedName(instance_name) + ".SingleInstance");
    impl->mutex = CreateMutexW(nullptr, TRUE, mutex_name.c_str());
    if (impl->mutex == nullptr) {
        impl->message = "failed to create single-instance mutex";
        return SingleInstanceLock{std::move(impl)};
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(impl->mutex);
        impl->mutex = nullptr;
        impl->message = "another LoFiBox Zero instance is already running";
        return SingleInstanceLock{std::move(impl)};
    }
    impl->acquired = true;
    impl->message = "single-instance lock acquired";
#elif defined(__linux__)
    impl->path = lockPathFor(instance_name);
    std::error_code directory_error{};
    std::filesystem::create_directories(impl->path.parent_path(), directory_error);
    impl->fd = open(impl->path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (impl->fd < 0) {
        impl->message = "failed to open single-instance lock: " + impl->path.string() + ": " + std::strerror(errno);
        return SingleInstanceLock{std::move(impl)};
    }
    if (flock(impl->fd, LOCK_EX | LOCK_NB) != 0) {
        impl->message = "another LoFiBox Zero instance is already running";
        return SingleInstanceLock{std::move(impl)};
    }
    if (ftruncate(impl->fd, 0) != 0) {
        impl->message = "failed to truncate single-instance lock: " + impl->path.string() + ": " + std::strerror(errno);
        return SingleInstanceLock{std::move(impl)};
    }
    const std::string pid = std::to_string(static_cast<long long>(getpid())) + "\n";
    const ssize_t written = write(impl->fd, pid.data(), pid.size());
    if (written < 0 || static_cast<std::size_t>(written) != pid.size()) {
        impl->message = "failed to write single-instance lock pid: " + impl->path.string() + ": " + std::strerror(errno);
        return SingleInstanceLock{std::move(impl)};
    }
    impl->acquired = true;
    impl->message = "single-instance lock acquired: " + impl->path.string();
#else
    impl->acquired = true;
    impl->message = "single-instance lock unsupported on this platform";
#endif
    return SingleInstanceLock{std::move(impl)};
}

bool SingleInstanceLock::acquired() const
{
    return impl_ != nullptr && impl_->acquired;
}

const std::string& SingleInstanceLock::message() const
{
    static const std::string empty{};
    return impl_ == nullptr ? empty : impl_->message;
}

} // namespace lofibox::platform::host
