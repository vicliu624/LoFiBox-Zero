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

#include "platform/host/png_canvas_loader.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;
class HostConnectivityProvider final : public app::ConnectivityProvider {
public:
    [[nodiscard]] bool connected() const override
    {
        return probeConnectivity();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "SYSTEM";
    }
};

} // namespace

std::shared_ptr<app::ConnectivityProvider> createHostConnectivityProvider()
{
    return std::make_shared<HostConnectivityProvider>();
}

} // namespace lofibox::platform::host
