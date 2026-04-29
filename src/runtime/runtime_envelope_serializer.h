// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>

#include "runtime/runtime_event.h"
#include "runtime/runtime_transport.h"

namespace lofibox::runtime {

[[nodiscard]] std::string runtimeCommandKindName(RuntimeCommandKind kind) noexcept;
[[nodiscard]] std::optional<RuntimeCommandKind> runtimeCommandKindFromName(std::string_view name) noexcept;
[[nodiscard]] std::string runtimeQueryKindName(RuntimeQueryKind kind) noexcept;
[[nodiscard]] std::optional<RuntimeQueryKind> runtimeQueryKindFromName(std::string_view name) noexcept;
[[nodiscard]] std::string commandOriginName(CommandOrigin origin) noexcept;
[[nodiscard]] std::optional<CommandOrigin> commandOriginFromName(std::string_view name) noexcept;

[[nodiscard]] std::string serializeRuntimeCommandRequest(const RuntimeCommandRequest& request);
[[nodiscard]] std::string serializeRuntimeCommandResponse(const RuntimeCommandResponse& response);
[[nodiscard]] std::string serializeRuntimeQueryRequest(const RuntimeQueryRequest& request);
[[nodiscard]] std::string serializeRuntimeQueryResponse(const RuntimeQueryResponse& response);
[[nodiscard]] std::string serializeRuntimeEventStreamRequest(const RuntimeEventStreamRequest& request);
[[nodiscard]] std::string serializeRuntimeEvent(const RuntimeEvent& event);

[[nodiscard]] std::optional<RuntimeCommandRequest> parseRuntimeCommandRequest(std::string_view json);
[[nodiscard]] std::optional<RuntimeCommandResponse> parseRuntimeCommandResponse(std::string_view json);
[[nodiscard]] std::optional<RuntimeQueryRequest> parseRuntimeQueryRequest(std::string_view json);
[[nodiscard]] std::optional<RuntimeQueryResponse> parseRuntimeQueryResponse(std::string_view json);
[[nodiscard]] std::optional<RuntimeEventStreamRequest> parseRuntimeEventStreamRequest(std::string_view json);
[[nodiscard]] std::optional<RuntimeEvent> parseRuntimeEvent(std::string_view json);

} // namespace lofibox::runtime
