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
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>
#include "core/operation_cancellation.h"

#include "platform/host/png_canvas_loader.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;
class HostConnectivityProvider final : public app::ConnectivityProvider {
public:
    HostConnectivityProvider() : worker_([this](std::stop_token stop) { monitor(stop); }) {}
    [[nodiscard]] bool connected() const override
    {
        return online_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "SYSTEM";
    }
private:
    void monitor(std::stop_token stop)
    {
        using clock = std::chrono::steady_clock;
        const core::OperationCancellationScope cancellation{stop};
#if defined(_WIN32)
        const auto curl = resolveExecutablePath(L"CURL_PATH", L"curl.exe");
        const auto python = resolveExecutablePath(L"PYTHON_PATH", L"python.exe");
        const std::string null_file = "NUL";
#else
        const auto curl = resolveExecutablePath("CURL_PATH", "curl");
        const auto python = resolveExecutablePath("PYTHON_PATH", "python3");
        const std::string null_file = "/dev/null";
#endif
        auto next_probe = clock::time_point{};
        bool had_route = false;
        std::mutex wait_mutex;
        std::condition_variable_any wake;
        while (!stop.stop_requested()) {
            const bool route = probeConnectivity();
            if (!route) online_ = false;
            if (route && (!had_route || clock::now() >= next_probe)) {
                bool reachable = false;
                if (curl || python) {
                    for (const auto* url : {"https://lrclib.net/", "https://musicbrainz.org/"}) {
                        if (stop.stop_requested()) break;
                        const auto response = curl ? captureProcessOutput(*curl, {
                            "--silent", "--head", "--output", null_file,
                            "--connect-timeout", "1", "--max-time", "2",
                            "--write-out", "%{http_code}", url})
                            : captureProcessOutput(*python, {"-c",
                                "import urllib.request,urllib.error,sys\n"
                                "try:\n"
                                " with urllib.request.urlopen(urllib.request.Request(sys.argv[1],method='HEAD'),timeout=2) as r: print(r.status)\n"
                                "except urllib.error.HTTPError as e: print(e.code)\n", url});
                        if (response && response->size() >= 3 && (*response)[0] >= '1' && (*response)[0] <= '5') {
                            reachable = true;
                            break;
                        }
                    }
                }
                online_ = reachable;
                next_probe = clock::now() + (reachable ? std::chrono::seconds{30} : std::chrono::seconds{15});
            }
            had_route = route;
            std::unique_lock lock{wait_mutex};
            wake.wait_for(lock, stop, std::chrono::seconds{1}, [] { return false; });
        }
    }
    std::atomic_bool online_{false};
    std::jthread worker_; // Destroy/join before the cached state.
};

} // namespace

std::shared_ptr<app::ConnectivityProvider> createHostConnectivityProvider()
{
    return std::make_shared<HostConnectivityProvider>();
}

} // namespace lofibox::platform::host
