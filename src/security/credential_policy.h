// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace lofibox::security {

struct CredentialRef {
    std::string id{};
};

struct TlsPolicy {
    bool verify_peer{true};
    bool allow_self_signed{false};
};

class SecretRedactor {
public:
    [[nodiscard]] std::string redact(std::string value) const;
};

} // namespace lofibox::security
