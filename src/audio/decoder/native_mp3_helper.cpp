// SPDX-License-Identifier: GPL-3.0-or-later

// Private PCM producer. Keep the FFmpeg producer's process/pipe ownership,
// including exec (not fork) when the existing decoder is required.
#include <mpg123.h>
#include <soxr.h>
#include "audio/pcm_format.h"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr long kInputRate = 44100;
constexpr long kRate = lofibox::audio::kPcmSampleRate;
constexpr int kChannels = lofibox::audio::kPcmChannels;
constexpr std::size_t kFrameBytes = lofibox::audio::kPcmFrameBytes;
struct Options {
    std::string input;
    double start_seconds{0};
    bool paced{true};
    std::vector<std::string> fallback;
};
std::optional<Options> parseOptions(int argc, char** argv)
{
    Options result;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (argument == "--input" && index + 1 < argc) result.input = argv[++index];
        else if (argument == "--start" && index + 1 < argc) {
            const std::string_view value{argv[++index]};
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result.start_seconds);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()
                || !std::isfinite(result.start_seconds) || result.start_seconds < 0
                || result.start_seconds > static_cast<double>(std::numeric_limits<int>::max()) / 1000.0) return std::nullopt;
        } else if (argument == "--unpaced") result.paced = false;
        else if (argument == "--") {
            while (++index < argc) result.fallback.emplace_back(argv[index]);
            break;
        } else return std::nullopt;
    }
    if (result.input.empty() || result.fallback.empty()) return std::nullopt;
    return result;
}
class FileOwner {
public:
    explicit FileOwner(const std::string& path) : fd_(::open(path.c_str(), O_RDONLY | O_CLOEXEC)) {}
    ~FileOwner() { if (fd_ >= 0) ::close(fd_); }
    FileOwner(const FileOwner&) = delete;
    FileOwner& operator=(const FileOwner&) = delete;
    int get() const { return fd_; }
private:
    int fd_{-1};
};
bool hasSingleMpegStream(int fd)
{
    // Accept only a complete Xing/Info stream whose declared byte extent
    // matches this file. A concatenation can change rate/channels after PCM
    // has already been emitted; route it to FFmpeg before producing anything.
    struct stat info{};
    if (::fstat(fd, &info) || !S_ISREG(info.st_mode) || info.st_size < 48) return false;
    std::array<unsigned char, 10> tag{};
    if (::pread(fd, tag.data(), tag.size(), 0) != static_cast<ssize_t>(tag.size())) return false;
    off_t first{};
    if (std::equal(tag.begin(), tag.begin() + 3, "ID3")) {
        if (tag[3] < 2 || tag[3] > 4) return false;
        first = 10;
        for (int index = 6; index < 10; ++index) {
            if (tag[index] & 0x80) return false;
            first += static_cast<off_t>(tag[index]) << (7 * (9 - index));
        }
        if (tag[3] == 4 && (tag[5] & 0x10)) first += 10;
    }
    std::array<unsigned char, 64> header{};
    if (first > info.st_size - static_cast<off_t>(header.size())
        || ::pread(fd, header.data(), header.size(), first) != static_cast<ssize_t>(header.size())) return false;
    const auto big_endian = [](const unsigned char* bytes) {
        return (std::uint32_t{bytes[0]} << 24) | (std::uint32_t{bytes[1]} << 16)
            | (std::uint32_t{bytes[2]} << 8) | bytes[3];
    };
    const auto word = big_endian(header.data());
    if ((word & 0xffe00000u) != 0xffe00000u || ((word >> 19) & 3) != 3
        || ((word >> 17) & 3) != 1 || ((word >> 10) & 3) != 0 || ((word >> 6) & 3) == 3
        || ((word >> 12) & 15) == 0 || ((word >> 12) & 15) == 15) return false;
    const std::size_t marker = 4 + ((word & 0x10000u) ? 0 : 2) + 32;
    const auto* xing = header.data() + marker;
    if (!std::equal(xing, xing + 4, "Xing") && !std::equal(xing, xing + 4, "Info")) return false;
    if ((big_endian(xing + 4) & 3) != 3 || !big_endian(xing + 8)) return false;
    off_t end = info.st_size;
    std::array<unsigned char, 3> tail{};
    if (end >= 128 && ::pread(fd, tail.data(), tail.size(), end - 128) == static_cast<ssize_t>(tail.size())
        && std::equal(tail.begin(), tail.end(), "TAG")) end -= 128;
    std::array<unsigned char, 32> ape{};
    if (end >= 32 && ::pread(fd, ape.data(), ape.size(), end - 32) == static_cast<ssize_t>(ape.size())
        && std::equal(ape.begin(), ape.begin() + 8, "APETAGEX")) {
        const auto little_endian = [](const unsigned char* bytes) {
            return std::uint32_t{bytes[0]} | (std::uint32_t{bytes[1]} << 8)
                | (std::uint32_t{bytes[2]} << 16) | (std::uint32_t{bytes[3]} << 24);
        };
        const auto version = little_endian(ape.data() + 8);
        const auto size = little_endian(ape.data() + 12);
        const auto flags = little_endian(ape.data() + 20);
        if (version != 2000 || size < 32 || size > end - first || (flags & 0x60000000u)) return false;
        end -= size;
        if (flags & 0x80000000u) {
            std::array<unsigned char, 32> ape_header{};
            if (end - first < 32 || ::pread(fd, ape_header.data(), ape_header.size(), end - 32)
                != static_cast<ssize_t>(ape_header.size())) return false;
            auto expected = ape;
            expected[23] |= 0x20; // APEv2 header flag, stored little endian.
            if (ape_header != expected) return false;
            end -= 32;
        }
    }
    return end - first == big_endian(xing + 12);
}
struct DecodeResult {
    bool fallback{true};
    double resume_seconds{0};
    int exit_code{0};
};
bool writePcm(const char* bytes, std::size_t size)
{
    while (size) {
        const auto written = ::write(STDOUT_FILENO, bytes, size);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return false;
        bytes += written;
        size -= static_cast<std::size_t>(written);
    }
    return true;
}
class Pacer {
public:
    void wait(std::uint64_t frames)
    {
        using Clock = std::chrono::steady_clock;
        const auto position = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>{static_cast<double>(frames) / kRate});
        const auto now = Clock::now();
        if (!anchor_) anchor_ = now;
        auto deadline = *anchor_ + position;
        // Resume after SIGSTOP or sink backpressure without a catch-up burst.
        if (now - deadline > std::chrono::milliseconds(100)) {
            anchor_ = now - position;
            deadline = now;
        }
        if (deadline > now) std::this_thread::sleep_until(deadline);
    }
private:
    std::optional<std::chrono::steady_clock::time_point> anchor_;
};
DecodeResult decode(const Options& options)
{
    if constexpr (std::endian::native != std::endian::little) return {true, options.start_seconds};
    FileOwner input{options.input};
    if (input.get() < 0 || !hasSingleMpegStream(input.get())) return {true, options.start_seconds};
    int error{};
    std::unique_ptr<mpg123_handle, decltype(&mpg123_delete)> decoder{mpg123_new(nullptr, &error), &mpg123_delete};
    if (!decoder) return {true, options.start_seconds};
    if (mpg123_param(decoder.get(), MPG123_ADD_FLAGS, MPG123_QUIET | MPG123_SKIP_ID3V2, 0) != MPG123_OK
        || mpg123_param(decoder.get(), MPG123_REMOVE_FLAGS, MPG123_AUTO_RESAMPLE, 0) != MPG123_OK
        || mpg123_param(decoder.get(), MPG123_RVA, MPG123_RVA_OFF, 0) != MPG123_OK
        // Rebuild the Layer III reservoir and synthesis history before a seek
        // target. The default pre-roll produced a transient in the PCM test.
        || mpg123_param(decoder.get(), MPG123_PREFRAMES, 16, 0) != MPG123_OK
        || mpg123_format_none(decoder.get()) != MPG123_OK) return {true, options.start_seconds};
    const long* rates{};
    std::size_t rate_count{};
    mpg123_rates(&rates, &rate_count);
    for (std::size_t index = 0; index < rate_count; ++index) {
        if (mpg123_format(decoder.get(), rates[index], MPG123_MONO | MPG123_STEREO, MPG123_ENC_FLOAT_32) != MPG123_OK)
            return {true, options.start_seconds};
    }
    if (mpg123_open_fd(decoder.get(), input.get()) != MPG123_OK) return {true, options.start_seconds};
    const auto supported_format = [&] {
        long rate{}; int channels{}, encoding{};
        return mpg123_getformat(decoder.get(), &rate, &channels, &encoding) == MPG123_OK
            && rate == kInputRate && channels == kChannels && encoding == MPG123_ENC_FLOAT_32;
    };
    if (!supported_format()) return {true, options.start_seconds};
    const auto target_frame = static_cast<std::uint64_t>(std::llround(options.start_seconds * kRate));
    std::uint64_t skip_frames = target_frame;
    if (options.start_seconds > 0) {
        // Restore both decoder and resampler history. Align to the rational
        // 147:160 conversion phase so seeking matches a slice of full PCM.
        const auto target_input = static_cast<off_t>(std::llround(options.start_seconds * kInputRate));
        const auto offset = std::max<off_t>(0, target_input - kInputRate / 4) / 147 * 147;
        const auto actual = mpg123_seek(decoder.get(), offset, SEEK_SET);
        if (actual != offset) return {true, options.start_seconds};
        skip_frames -= static_cast<std::uint64_t>(actual / 147) * 160;
    }
    const double base_seconds = static_cast<double>(target_frame) / kRate;
    const auto io = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
    const auto quality = soxr_quality_spec(SOXR_HQ, 0);
    const auto runtime = soxr_runtime_spec(1);
    soxr_error_t resample_error{};
    std::unique_ptr<soxr, decltype(&soxr_delete)> resampler{
        soxr_create(kInputRate, kRate, kChannels, &resample_error, &io, &quality, &runtime), &soxr_delete};
    if (!resampler || resample_error) return {true, options.start_seconds};
    std::array<float, 2048> pcm{};
    std::array<float, 4096> converted{};
    std::uint64_t frames{};
    int empty_reads{};
    Pacer pacer;
    const auto emit = [&](std::size_t count) {
        const auto dropped = std::min<std::uint64_t>(skip_frames, count);
        skip_frames -= dropped;
        count -= dropped;
        if (!count) return true;
        if (options.paced) pacer.wait(frames);
        if (!writePcm(reinterpret_cast<const char*>(converted.data() + dropped * kChannels), count * kFrameBytes)) return false;
        frames += count;
        return true;
    };
    while (true) {
        std::size_t bytes{};
        const int status = mpg123_read(decoder.get(), reinterpret_cast<unsigned char*>(pcm.data()), sizeof(pcm), &bytes);
        const double resume = base_seconds + static_cast<double>(frames) / kRate;
        if (status == MPG123_NEW_FORMAT && !supported_format()) return {true, resume};
        if (bytes % kFrameBytes) return {true, resume};
        if (bytes) {
            const auto input_frames = bytes / kFrameBytes;
            std::size_t consumed{};
            while (consumed < input_frames) {
                std::size_t used{}, produced{};
                if (soxr_process(resampler.get(), pcm.data() + consumed * kChannels, input_frames - consumed, &used,
                        converted.data(), converted.size() / kChannels, &produced) || (!used && !produced))
                    return {true, base_seconds + static_cast<double>(frames) / kRate};
                consumed += used;
                if (!emit(produced)) return {false, 0, 1};
            }
            empty_reads = 0;
        } else ++empty_reads;
        if (status == MPG123_DONE) {
            // EOF must drain the filter tail; omitting it truncates every song.
            while (true) {
                std::size_t produced{};
                if (soxr_process(resampler.get(), nullptr, 0, nullptr, converted.data(), converted.size() / kChannels, &produced))
                    return {true, base_seconds + static_cast<double>(frames) / kRate};
                if (!produced) return frames ? DecodeResult{false, 0, 0} : DecodeResult{true, options.start_seconds};
                if (!emit(produced)) return {false, 0, 1};
            }
        }
        if ((status != MPG123_OK && status != MPG123_NEW_FORMAT) || empty_reads > 8)
            return {true, base_seconds + static_cast<double>(frames) / kRate};
    }
}
int fallback(Options& options, double resume_seconds)
{
    if (resume_seconds != options.start_seconds) {
        std::ostringstream value;
        value.imbue(std::locale::classic());
        value << std::fixed << std::setprecision(6) << resume_seconds;
        auto input = std::find(options.fallback.begin(), options.fallback.end(), "-i");
        auto seek = std::find(options.fallback.begin(), input, "-ss");
        if (seek != input && seek + 1 != input) *(seek + 1) = value.str();
        else options.fallback.insert(input, {"-ss", value.str()});
    }
    std::vector<char*> args;
    for (auto& value : options.fallback) args.push_back(value.data());
    args.push_back(nullptr);
    ::execv(args.front(), args.data());
    return 127;
}
} // namespace
int main(int argc, char** argv)
{
    auto options = parseOptions(argc, argv);
    if (!options) return 2;
    if (mpg123_init() != MPG123_OK) return fallback(*options, options->start_seconds);
    const auto result = decode(*options);
    // decode() destroyed its mpg123 handle and caller-owned CLOEXEC FD before
    // exec. The replacement keeps this PID, process group and PCM pipe.
    mpg123_exit();
    return result.fallback ? fallback(*options, result.resume_seconds) : result.exit_code;
}
