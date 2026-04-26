// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/playback_backend_controller.h"

namespace lofibox::app {

void PlaybackBackendController::setServices(RuntimeServices* services) noexcept
{
    services_ = services;
}

bool PlaybackBackendController::startFile(const std::filesystem::path& path, double start_seconds)
{
    auto* audio_backend = backend();
    return audio_backend != nullptr && audio_backend->playFile(path, start_seconds);
}

void PlaybackBackendController::pause() noexcept
{
    if (auto* audio_backend = backend()) {
        audio_backend->pause();
    }
}

void PlaybackBackendController::resume() noexcept
{
    if (auto* audio_backend = backend()) {
        audio_backend->resume();
    }
}

void PlaybackBackendController::stop() noexcept
{
    if (auto* audio_backend = backend()) {
        audio_backend->stop();
    }
}

AudioPlaybackState PlaybackBackendController::state() const
{
    auto* audio_backend = backend();
    return audio_backend == nullptr ? AudioPlaybackState::Idle : audio_backend->state();
}

AudioVisualizationFrame PlaybackBackendController::visualizationFrame() const
{
    auto* audio_backend = backend();
    return audio_backend == nullptr ? AudioVisualizationFrame{} : audio_backend->visualizationFrame();
}

AudioPlaybackBackend* PlaybackBackendController::backend() const noexcept
{
    if (services_ == nullptr || !services_->playback.audio_backend) {
        return nullptr;
    }
    return services_->playback.audio_backend.get();
}

} // namespace lofibox::app
