// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/app_service_registry.h"

namespace lofibox::application {

AppServiceRegistry::AppServiceRegistry(app::AppControllerSet& controllers, app::RuntimeServices& runtime_services) noexcept
    : controllers_(controllers),
      runtime_services_(runtime_services)
{
}

PlaybackCommandService AppServiceRegistry::playbackCommands() const noexcept
{
    return PlaybackCommandService{controllers_.playback, controllers_.library};
}

QueueCommandService AppServiceRegistry::queueCommands() const noexcept
{
    return QueueCommandService{playbackCommands()};
}

PlaybackStatusQueryService AppServiceRegistry::playbackStatus() const noexcept
{
    return PlaybackStatusQueryService{controllers_.playback, controllers_.library};
}

LibraryQueryService AppServiceRegistry::libraryQueries() const noexcept
{
    return LibraryQueryService{controllers_.library};
}

LibraryMutationService AppServiceRegistry::libraryMutations() const noexcept
{
    return LibraryMutationService{controllers_.library};
}

LibraryOpenActionService AppServiceRegistry::libraryOpenActions() const noexcept
{
    return LibraryOpenActionService{controllers_.library};
}

SourceProfileCommandService AppServiceRegistry::sourceProfiles() const noexcept
{
    return SourceProfileCommandService{runtime_services_};
}

} // namespace lofibox::application
