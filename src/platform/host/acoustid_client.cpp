// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_enrichment_client_helpers.h"
#include "platform/host/runtime_logger.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {
namespace {

std::string environmentValue(const char* name)
{
#if defined(_WIN32)
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return {};
    }
    std::string result{value};
    std::free(value);
    return result;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

} // namespace

ChromaprintFingerprintProvider::ChromaprintFingerprintProvider()
{
#if defined(_WIN32)
    executable_path_ = resolveExecutablePath(L"FPCALC_PATH", L"fpcalc.exe");
#elif defined(__linux__)
    executable_path_ = resolveExecutablePath("FPCALC_PATH", "fpcalc");
#endif
}

bool ChromaprintFingerprintProvider::available() const
{
    return executable_path_.has_value();
}

AudioFingerprintResult ChromaprintFingerprintProvider::fingerprint(const fs::path& path) const
{
    AudioFingerprintResult result{};
    if (!available() || !fs::exists(path)) {
        return result;
    }

    const auto output = captureProcessOutput(*executable_path_, {pathUtf8String(path)});
    if (!output) {
        return result;
    }

    std::size_t start = 0;
    while (start <= output->size()) {
        const auto end = output->find('\n', start);
        const auto line = output->substr(start, end == std::string::npos ? std::string::npos : end - start);
        const auto separator = line.find('=');
        if (separator != std::string::npos) {
            const auto key = line.substr(0, separator);
            const auto value = trim(line.substr(separator + 1));
            if (key == "DURATION") {
                try {
                    result.duration_seconds = static_cast<int>(std::lround(std::stod(value)));
                } catch (...) {
                    result.duration_seconds = 0;
                }
            } else if (key == "FINGERPRINT") {
                result.fingerprint = value;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    result.found = !result.fingerprint.empty() && result.duration_seconds > 0;
    return result;
}

AcoustIdIdentityClient::AcoustIdIdentityClient()
{
#if defined(_WIN32)
    curl_path_ = resolveExecutablePath(L"CURL_PATH", L"curl.exe");
#elif defined(__linux__)
    curl_path_ = resolveExecutablePath("CURL_PATH", "curl");
#endif
    if (const auto api_key = environmentValue("ACOUSTID_API_KEY"); !api_key.empty()) {
        api_key_ = api_key;
    } else if (const auto client_key = environmentValue("ACOUSTID_CLIENT_KEY"); !client_key.empty()) {
        api_key_ = client_key;
    } else {
        api_key_ = std::string(kDefaultAcoustIdClientKey);
    }
}

bool AcoustIdIdentityClient::available() const
{
    return !api_key_.empty()
        && fingerprint_provider_.available()
        && (curl_path_.has_value() || resolvePythonPath().has_value());
}

app::TrackIdentity AcoustIdIdentityClient::lookup(const fs::path& path, const app::TrackMetadata& seed_metadata) const
{
    app::TrackIdentity identity{};
    if (!available()) {
        if (api_key_.empty()) {
            logRuntime(RuntimeLogLevel::Debug, "identity", "AcoustID skipped: missing ACOUSTID_API_KEY/ACOUSTID_CLIENT_KEY");
        } else if (!fingerprint_provider_.available()) {
            logRuntime(RuntimeLogLevel::Debug, "identity", "AcoustID skipped: fpcalc unavailable");
        }
        return identity;
    }

    const auto fingerprint = fingerprint_provider_.fingerprint(path);
    if (!fingerprint.found) {
        logRuntime(RuntimeLogLevel::Warn, "identity", "Chromaprint fingerprint failed for " + pathUtf8String(path));
        return identity;
    }

    logRuntime(RuntimeLogLevel::Debug, "identity", "AcoustID lookup started for " + pathUtf8String(path));
    const std::vector<std::pair<std::string, std::string>> fields{
        {"client", api_key_},
        {"meta", "recordings releasegroups releases"},
        {"duration", std::to_string(fingerprint.duration_seconds)},
        {"fingerprint", fingerprint.fingerprint}};
    if (const auto json = captureFormPost(curl_path_, "https://api.acoustid.org/v2/lookup", fields)) {
        identity = parseBestAcoustIdIdentity(*json, seed_metadata);
        if (identity.found) {
            if (!identity.metadata.duration_seconds) {
                identity.metadata.duration_seconds = fingerprint.duration_seconds;
            }
            logRuntime(RuntimeLogLevel::Info, "identity", "AcoustID identity hit for " + pathUtf8String(path));
        } else {
            logRuntime(RuntimeLogLevel::Warn, "identity", "AcoustID identity miss for " + pathUtf8String(path));
        }
    } else {
        logRuntime(RuntimeLogLevel::Warn, "identity", "AcoustID request failed for " + pathUtf8String(path));
    }
    return identity;
}

} // namespace lofibox::platform::host::runtime_detail

