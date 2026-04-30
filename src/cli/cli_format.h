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

inline bool containsField(const std::vector<std::string>& fields, std::string_view name)
{
    return std::find(fields.begin(), fields.end(), name) != fields.end();
}

inline std::string joinFields(const std::vector<std::string>& fields)
{
    std::ostringstream out{};
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (index > 0U) {
            out << ',';
        }
        out << fields[index];
    }
    return out.str();
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

inline std::vector<std::string> fieldNames(const CliFields& fields)
{
    std::vector<std::string> names{};
    names.reserve(fields.size());
    for (const auto& [name, value] : fields) {
        (void)value;
        names.push_back(name);
    }
    return names;
}

inline std::vector<std::string> unknownRequestedFields(const CliOutputOptions& options, const std::vector<std::string>& allowed)
{
    std::vector<std::string> unknown{};
    for (const auto& field : options.fields) {
        if (!containsField(allowed, field)) {
            unknown.push_back(field);
        }
    }
    return unknown;
}

inline void printJsonStringVector(std::ostream& out, std::string_view name, const std::vector<std::string>& values, bool& first)
{
    if (!first) {
        out << ',';
    }
    first = false;
    printJsonString(out, name);
    out << ":[";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0U) {
            out << ',';
        }
        printJsonString(out, values[index]);
    }
    out << ']';
}

inline void printUnknownFieldsWarning(
    std::ostream& out,
    const CliOutputOptions& options,
    const std::vector<std::string>& allowed,
    bool& first)
{
    if (options.fields.empty()) {
        return;
    }
    const auto unknown = unknownRequestedFields(options, allowed);
    if (unknown.empty()) {
        return;
    }
    printJsonField(out, "_warning", "unknown fields ignored: " + joinFields(unknown), first);
    printJsonStringVector(out, "_unknown_fields", unknown, first);
    printJsonStringVector(out, "_allowed_fields", allowed, first);
}

struct CliStructuredError {
    std::string code{"ERROR"};
    std::string message{};
    std::string argument{};
    std::string expected{};
    int exit_code{static_cast<int>(CliExitCode::Usage)};
    std::string usage{};
    std::vector<std::string> allowed{};
};

inline void printCliError(std::ostream& err, const CliOutputOptions& options, const CliStructuredError& error)
{
    if (!options.json) {
        err << error.message << '\n';
        if (!error.allowed.empty()) {
            err << "Allowed fields: " << joinFields(error.allowed) << '\n';
        }
        if (!error.usage.empty()) {
            err << "Usage: " << error.usage << '\n';
        }
        return;
    }
    err << "{\"error\":{";
    bool first = true;
    printJsonField(err, "code", error.code, first);
    printJsonField(err, "message", error.message, first);
    if (!error.argument.empty()) {
        printJsonField(err, "argument", error.argument, first);
    }
    if (!error.expected.empty()) {
        printJsonField(err, "expected", error.expected, first);
    }
    printJsonNumberField(err, "exit_code", std::to_string(error.exit_code), first);
    if (!error.usage.empty()) {
        printJsonField(err, "usage", error.usage, first);
    }
    if (!error.allowed.empty()) {
        if (!first) {
            err << ',';
        }
        first = false;
        printJsonString(err, "allowed");
        err << ":[";
        for (std::size_t index = 0; index < error.allowed.size(); ++index) {
            if (index > 0U) {
                err << ',';
            }
            printJsonString(err, error.allowed[index]);
        }
        err << ']';
    }
    err << "}}\n";
}

inline void printObject(std::ostream& out, const CliOutputOptions& options, const CliFields& fields)
{
    if (options.quiet) {
        return;
    }
    if (options.json) {
        out << '{';
        bool first = true;
        const auto allowed = fieldNames(fields);
        printUnknownFieldsWarning(out, options, allowed, first);
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
        const auto allowed = fieldNames(rows.front());
        const auto unknown = unknownRequestedFields(options, allowed);
        if (!unknown.empty()) {
            out << '{';
            bool first = true;
            printJsonField(out, "_warning", "unknown fields ignored: " + joinFields(unknown), first);
            printJsonStringVector(out, "_unknown_fields", unknown, first);
            printJsonStringVector(out, "_allowed_fields", allowed, first);
            if (!first) {
                out << ',';
            }
            printJsonString(out, "rows");
            out << ':';
        }
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
        out << ']';
        if (!unknown.empty()) {
            out << '}';
        }
        out << '\n';
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
