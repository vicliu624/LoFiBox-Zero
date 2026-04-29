// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lofibox::cli {

enum class CliExitCode : int {
    Success = 0,
    Usage = 2,
    NotFound = 3,
    Credential = 4,
    Network = 5,
    Playback = 6,
    Persistence = 7,
};

struct CliOutputOptions {
    bool json{false};
    bool porcelain{false};
    bool quiet{false};
    std::vector<std::string> fields{};
};

using CliFields = std::vector<std::pair<std::string, std::string>>;

inline std::vector<std::string> splitFields(std::string_view value)
{
    std::vector<std::string> fields{};
    std::size_t start = 0;
    while (start <= value.size()) {
        const auto comma = value.find(',', start);
        const auto end = comma == std::string_view::npos ? value.size() : comma;
        if (end > start) {
            fields.emplace_back(value.substr(start, end - start));
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1U;
    }
    return fields;
}

inline bool wantsField(const CliOutputOptions& options, std::string_view name)
{
    return options.fields.empty()
        || std::find(options.fields.begin(), options.fields.end(), name) != options.fields.end();
}

inline std::string jsonEscape(std::string_view value)
{
    std::ostringstream out{};
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20U) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
            } else {
                out << static_cast<char>(ch);
            }
            break;
        }
    }
    return out.str();
}

inline void printJsonString(std::ostream& out, std::string_view value)
{
    out << '"' << jsonEscape(value) << '"';
}

inline void printJsonField(std::ostream& out, std::string_view name, std::string_view value, bool& first)
{
    if (!first) {
        out << ',';
    }
    first = false;
    printJsonString(out, name);
    out << ':';
    printJsonString(out, value);
}

inline void printJsonBoolField(std::ostream& out, std::string_view name, bool value, bool& first)
{
    if (!first) {
        out << ',';
    }
    first = false;
    printJsonString(out, name);
    out << ':' << (value ? "true" : "false");
}

inline void printJsonNumberField(std::ostream& out, std::string_view name, std::string_view value, bool& first)
{
    if (!first) {
        out << ',';
    }
    first = false;
    printJsonString(out, name);
    out << ':' << value;
}

inline void printObject(std::ostream& out, const CliOutputOptions& options, const CliFields& fields)
{
    if (options.quiet) {
        return;
    }
    if (options.json) {
        out << '{';
        bool first = true;
        for (const auto& [name, value] : fields) {
            if (wantsField(options, name)) {
                printJsonField(out, name, value, first);
            }
        }
        out << "}\n";
        return;
    }
    for (const auto& [name, value] : fields) {
        if (!wantsField(options, name)) {
            continue;
        }
        if (options.porcelain) {
            out << name << '\t' << value << '\n';
        } else {
            out << name << ": " << value << '\n';
        }
    }
}

inline void printPorcelainRows(
    std::ostream& out,
    const CliOutputOptions& options,
    const std::vector<CliFields>& rows,
    std::string_view empty_label = "NO RESULTS")
{
    if (options.quiet) {
        return;
    }
    if (rows.empty()) {
        if (options.json) {
            out << "[]\n";
        } else {
            out << empty_label << '\n';
        }
        return;
    }
    if (options.json) {
        out << '[';
        bool first_row = true;
        for (const auto& row : rows) {
            if (!first_row) {
                out << ',';
            }
            first_row = false;
            out << '{';
            bool first_field = true;
            for (const auto& [name, value] : row) {
                if (wantsField(options, name)) {
                    printJsonField(out, name, value, first_field);
                }
            }
            out << '}';
        }
        out << "]\n";
        return;
    }
    for (const auto& row : rows) {
        bool first = true;
        for (const auto& [name, value] : row) {
            if (!wantsField(options, name)) {
                continue;
            }
            if (!first) {
                out << (options.porcelain ? "\t" : ", ");
            }
            first = false;
            if (options.porcelain) {
                out << value;
            } else {
                out << name << ": " << value;
            }
        }
        out << '\n';
    }
}

} // namespace lofibox::cli
