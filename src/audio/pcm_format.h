// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>

namespace lofibox::audio {
// Raw PCM has no header. Every producer, DSP clock and output sink must use
// this same contract, independently of the source file's native sample rate.
inline constexpr int kPcmSampleRate = 48000;
inline constexpr int kPcmChannels = 2;
inline constexpr std::size_t kPcmFrameBytes = sizeof(float) * kPcmChannels;
} // namespace lofibox::audio
