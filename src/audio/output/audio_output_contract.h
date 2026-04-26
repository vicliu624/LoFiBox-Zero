// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <span>
#include <string>

namespace lofibox::audio::output {

struct AudioOutputFormat {
    int sample_rate_hz{0};
    int channels{0};
};

class AudioOutputSink {
public:
    virtual ~AudioOutputSink() = default;

    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual std::string displayName() const = 0;
    [[nodiscard]] virtual bool open(AudioOutputFormat format) = 0;
    [[nodiscard]] virtual bool write(std::span<const float> interleaved_samples) = 0;
    virtual void pause() noexcept = 0;
    virtual void resume() noexcept = 0;
    virtual void close() noexcept = 0;
};

} // namespace lofibox::audio::output
