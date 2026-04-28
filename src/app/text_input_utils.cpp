// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/text_input_utils.h"

#include <algorithm>

namespace lofibox::app {
namespace {

[[nodiscard]] bool isContinuation(unsigned char byte) noexcept
{
    return (byte & 0xC0U) == 0x80U;
}

[[nodiscard]] std::size_t utf8SequenceLength(unsigned char lead) noexcept
{
    if (lead <= 0x7FU) {
        return 1U;
    }
    if (lead >= 0xC2U && lead <= 0xDFU) {
        return 2U;
    }
    if (lead >= 0xE0U && lead <= 0xEFU) {
        return 3U;
    }
    if (lead >= 0xF0U && lead <= 0xF4U) {
        return 4U;
    }
    return 0U;
}

[[nodiscard]] bool validCodepointAt(std::string_view text, std::size_t offset, std::size_t& length) noexcept
{
    const auto lead = static_cast<unsigned char>(text[offset]);
    length = utf8SequenceLength(lead);
    if (length == 0U || offset + length > text.size()) {
        return false;
    }
    for (std::size_t index = 1U; index < length; ++index) {
        if (!isContinuation(static_cast<unsigned char>(text[offset + index]))) {
            return false;
        }
    }

    if (length == 3U) {
        const auto second = static_cast<unsigned char>(text[offset + 1U]);
        if ((lead == 0xE0U && second < 0xA0U) || (lead == 0xEDU && second >= 0xA0U)) {
            return false;
        }
    } else if (length == 4U) {
        const auto second = static_cast<unsigned char>(text[offset + 1U]);
        if ((lead == 0xF0U && second < 0x90U) || (lead == 0xF4U && second > 0x8FU)) {
            return false;
        }
    }
    return true;
}

} // namespace

bool isValidUtf8(std::string_view text) noexcept
{
    std::size_t offset = 0U;
    while (offset < text.size()) {
        std::size_t length = 0U;
        if (!validCodepointAt(text, offset, length)) {
            return false;
        }
        offset += length;
    }
    return true;
}

std::string truncateUtf8(std::string_view text, std::size_t max_bytes)
{
    std::string result{};
    result.reserve(std::min(text.size(), max_bytes));
    std::size_t offset = 0U;
    while (offset < text.size() && result.size() < max_bytes) {
        std::size_t length = 0U;
        if (!validCodepointAt(text, offset, length)) {
            break;
        }
        if (result.size() + length > max_bytes) {
            break;
        }
        result.append(text.substr(offset, length));
        offset += length;
    }
    return result;
}

bool appendUtf8Bounded(std::string& target, std::string_view committed_text, std::size_t max_bytes)
{
    if (committed_text.empty() || target.size() >= max_bytes || !isValidUtf8(target)) {
        return false;
    }
    const auto remaining = max_bytes - target.size();
    const auto appendable = truncateUtf8(committed_text, remaining);
    if (appendable.empty() && !committed_text.empty()) {
        return false;
    }
    target += appendable;
    return !appendable.empty();
}

bool popLastUtf8Codepoint(std::string& target)
{
    if (target.empty()) {
        return false;
    }
    if (!isValidUtf8(target)) {
        target.pop_back();
        return true;
    }

    std::size_t erase_at = target.size() - 1U;
    while (erase_at > 0U && isContinuation(static_cast<unsigned char>(target[erase_at]))) {
        --erase_at;
    }
    target.erase(erase_at);
    return true;
}

} // namespace lofibox::app
