// SPDX-License-Identifier: GPL-3.0-or-later

#include "security/credential_policy.h"

#include <array>

namespace lofibox::security {

std::string SecretRedactor::redact(std::string value) const
{
    static constexpr std::array<std::string_view, 5> markers{
        "password=",
        "api_key=",
        "token=",
        "access_token=",
        "Authorization:"};
    for (const auto marker : markers) {
        std::size_t pos = 0;
        while ((pos = value.find(marker, pos)) != std::string::npos) {
            const auto begin = pos + marker.size();
            auto end = value.find_first_of("& \n\r\t", begin);
            if (end == std::string::npos) {
                end = value.size();
            }
            value.replace(begin, end - begin, "<redacted>");
            pos = begin + 10;
        }
    }
    return value;
}

} // namespace lofibox::security
