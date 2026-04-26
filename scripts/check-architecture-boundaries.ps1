# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [switch]$Quiet
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Convert-ToRepoPath([string]$Path)
{
    return $Path.Substring($repo.Length + 1).Replace("\", "/")
}

function Is-SourceFile([System.IO.FileInfo]$File)
{
    return @(".c", ".cc", ".cpp", ".h", ".hpp") -contains $File.Extension
}

function Test-AnyPrefix([string]$Value, [string[]]$Prefixes)
{
    foreach ($prefix in $Prefixes) {
        if ($Value.StartsWith($prefix, [System.StringComparison]::Ordinal)) {
            return $true
        }
    }
    return $false
}

function Add-Violation([System.Collections.Generic.List[string]]$Violations, [string]$Path, [int]$Line, [string]$Include, [string]$Reason)
{
    $Violations.Add("${Path}:${Line}: ${Include} - ${Reason}")
}

$includeRegex = '^\s*#\s*include\s+[<"]([^>"]+)[>"]'
$violations = [System.Collections.Generic.List[string]]::new()

Get-ChildItem -Path (Join-Path $repo "src") -Recurse -File | Where-Object { Is-SourceFile $_ } | ForEach-Object {
    $repoPath = Convert-ToRepoPath $_.FullName
    $lines = Get-Content -LiteralPath $_.FullName

    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = $lines[$index]
        $lineNumber = $index + 1

        if ($repoPath.StartsWith("src/ui/", [System.StringComparison]::Ordinal) -and $line -match '\bapp::') {
            Add-Violation $violations $repoPath $lineNumber "app::" "UI code must use UI projection types and must not reference app namespace types"
        }

        if ($repoPath -eq "src/app/lofibox_app.cpp") {
            if ($line -match 'case\s+\d+\s*:|selected\s*==\s*\d+|std::clamp|\.startTrack\s*\(|\.openSelectedListItem\s*\(|\.setShuffleEnabled\s*\(|\.setRepeatOne\s*\(|\.setRepeatAll\s*\(|\.cycleSongSortMode\s*\(') {
                Add-Violation $violations $repoPath $lineNumber "command semantics" "LoFiBoxApp must delegate page command semantics to app_command_executor instead of owning menu/list/playback/EQ branching"
            }
        }

        if ($repoPath -eq "src/app/app_runtime_context.h") {
            if ($line -match '^\s*(std::vector<std::filesystem::path>|ui::UiAssets|LibraryController|PlaybackController|NavigationState|SettingsState|NetworkState|MetadataServiceState|EqState)\s+[A-Za-z0-9_]+\{') {
                Add-Violation $violations $repoPath $lineNumber "runtime field ownership" "AppRuntimeContext must not re-accumulate raw runtime state or controllers; use AppRuntimeState and AppControllerSet"
            }
        }

        if ($repoPath -eq "src/app/playback_controller.cpp" -or $repoPath -eq "src/app/playback_controller.h") {
            if ($line -match '\bstd::thread\b|\bstd::mutex\b|pending_enrichment|enrichment_threads|requestEnrichment|applyPendingEnrichments|PlaybackSessionClock|PlaybackBackendController|PlaybackTransitionCoordinator|PlaybackCompletionPolicy|AudioPipeline|AudioPlaybackState|visualizationFrame\(') {
                Add-Violation $violations $repoPath $lineNumber "playback core ownership" "PlaybackController must own playback commands and queue semantics only; enrichment, clock, backend, transition, completion policy, and audio pipeline responsibilities belong to dedicated integrated-core components"
            }
        }

        if ($repoPath -eq "src/app/playback_runtime_coordinator.cpp" -or $repoPath -eq "src/app/playback_runtime_coordinator.h") {
            if ($line -match 'PlaybackBackendController|PlaybackVisualizationSource') {
                Add-Violation $violations $repoPath $lineNumber "audio pipeline facade" "PlaybackRuntimeCoordinator must use the AudioPipelineController facade instead of backend/visualization adapters directly"
            }
        }

        if ($repoPath -eq "src/platform/host/runtime_enrichment_clients.cpp") {
            if ($line -match '\b(AcoustIdIdentityClient|MusicBrainzMetadataClient|CoverArtArchiveClient|LrclibClient|LyricsOvhClient|FfprobeMetadataReader)::') {
                Add-Violation $violations $repoPath $lineNumber "enrichment protocol client" "runtime_enrichment_clients.cpp must stay shared helper/orchestration code; concrete protocol clients live in dedicated files"
            }
        }

        if ($repoPath -eq "src/platform/host/metadata_provider.cpp") {
            if ($line -match '\b(MusicBrainzMetadataClient|AcoustIdIdentityClient)::|\bMusicBrainzMetadataClient\s+\w+') {
                Add-Violation $violations $repoPath $lineNumber "metadata enrichment orchestration" "metadata_provider.cpp must delegate online lookup ordering to runtime_metadata_enrichment_orchestrator"
            }
        }

        if ($repoPath -eq "src/platform/host/runtime_services_factory.cpp") {
            if ($line -match '\bcreateHost(MetadataProvider|TrackIdentityProvider|ArtworkProvider|LyricsProvider|TagWriter|AudioPlaybackBackend|ConnectivityProvider|RemoteSourceProvider|RemoteCatalogProvider|RemoteStreamResolver)\s*\(') {
                Add-Violation $violations $repoPath $lineNumber "runtime service provider" "runtime_services_factory.cpp must compose host service providers and must not directly construct concrete runtime capabilities"
            }
        }

        if ($repoPath -eq "src/platform/host/lyrics_provider.cpp") {
            if ($line -match 'class\s+LyricsCache|class\s+LyricsWritebackPolicy') {
                Add-Violation $violations $repoPath $lineNumber "lyrics pipeline component" "LyricsProvider must compose lyrics pipeline components instead of owning cache/writeback policy internals"
            }
        }

        if ($repoPath -eq "src/app/app_renderer.cpp") {
            if ($line -match '\b(toUiSpectrumFrame|toUiLyricsContent|toUiPlaybackStatus|toUiIndexState|playbackSummary|emptyLabelForPage|formatStorage)\b') {
                Add-Violation $violations $repoPath $lineNumber "projection builder ownership" "AppRenderer must dispatch pages only; app-state to UI projection belongs to AppProjectionBuilder"
            }
        }

        if ($repoPath -eq "src/app/library_controller.cpp") {
            if ($line -match 'case\s+AppPage::|case\s+\d+\s*:|LibraryDatabase|LibraryStore|LibraryIndexer|LibraryMutationService|LibraryScanScheduler|RemoteSource|RemoteSourceRegistry') {
                Add-Violation $violations $repoPath $lineNumber "library fact ownership" "LibraryController must not own list-open branching, durable library storage/indexing, or remote provider access"
            }
        }

        if ($repoPath -eq "src/app/library_controller.h") {
            if ($line -match '^\s*(LibraryIndexState|LibraryModel)\s+\w+_\{') {
                Add-Violation $violations $repoPath $lineNumber "library repository ownership" "LibraryController must keep library fact storage behind LibraryRepository"
            }
        }

        if ($repoPath.StartsWith("src/remote/", [System.StringComparison]::Ordinal)) {
            if ($line -match '\bPlaybackController\b|\bPlaybackSession\b|ui::|pages::') {
                Add-Violation $violations $repoPath $lineNumber "remote source boundary" "Remote providers must produce catalog/stream facts and must not control playback or UI"
            }
        }

        if ($repoPath.StartsWith("src/metadata/", [System.StringComparison]::Ordinal)) {
            if ($line -match 'ui::|pages::|PlaybackController') {
                Add-Violation $violations $repoPath $lineNumber "metadata boundary" "Metadata/enrichment code must not depend on UI pages or playback controllers"
            }
        }

        if ($repoPath.StartsWith("src/desktop/", [System.StringComparison]::Ordinal)) {
            if ($line -match 'ui::|pages::|AudioPlaybackBackend|RemoteSourceProvider|RemoteCatalogProvider|RemoteStreamResolver') {
                Add-Violation $violations $repoPath $lineNumber "desktop projection boundary" "Desktop integration must consume app commands/projections and must not depend on UI pages, playback backends, or remote providers"
            }
        }

        if ($repoPath -eq "src/ui/pages/lyrics_page.cpp") {
            if ($line -match '^\s*(void|int|std::optional|std::vector<|struct)\s+(renderFoamSide|renderSideFoamSpectrum|parseLyricTimestamp|lyricDisplayLines|activeLyricIndex|LyricDisplayLine)\b|struct\s+LyricDisplayLine') {
                Add-Violation $violations $repoPath $lineNumber "lyrics page algorithm ownership" "LyricsPage must compose layout/effect widgets; lyric parsing and spectrum algorithms belong to ui/widgets or ui/effects"
            }
        }

        if ($line -match '\bservices(_snapshot|_)?\.(metadata_provider|track_identity_provider|artwork_provider|audio_playback_backend|lyrics_provider|tag_writer|connectivity_provider|remote_source_provider|remote_catalog_provider|remote_stream_resolver)\b') {
            Add-Violation $violations $repoPath $lineNumber "runtime service grouping" "Runtime services must be accessed through capability groups: connectivity, metadata, playback, or remote"
        }

        if ($line -notmatch $includeRegex) {
            continue
        }

        $include = $Matches[1]

        if ($repoPath.StartsWith("src/core/", [System.StringComparison]::Ordinal)) {
            if (Test-AnyPrefix $include @("app/", "platform/", "audio/", "metadata/", "library/", "playback/", "remote/", "desktop/", "security/", "ui/")) {
                Add-Violation $violations $repoPath $lineNumber $include "core must not depend on higher layers"
            }
        }

        if ($repoPath.StartsWith("src/ui/", [System.StringComparison]::Ordinal)) {
            if (Test-AnyPrefix $include @("app/", "platform/", "audio/", "metadata/", "library/", "playback/", "remote/", "desktop/", "security/")) {
                Add-Violation $violations $repoPath $lineNumber $include "UI code must use UI projection types and must not include app, platform, or backend layers"
            }
        }

        if ($repoPath.StartsWith("src/app/", [System.StringComparison]::Ordinal) -and -not $repoPath.Contains("_runner.")) {
            if (Test-AnyPrefix $include @("platform/host/", "platform/device/", "platform/x11/")) {
                Add-Violation $violations $repoPath $lineNumber $include "shared app code must not include concrete platform adapters"
            }
        }

        if ($repoPath -eq "src/app/lofibox_app.cpp") {
            if (Test-AnyPrefix $include @("app/input_actions.h", "core/bitmap_font.h", "core/color.h", "ui/pages/", "ui/ui_primitives.h", "ui/ui_theme.h")) {
                Add-Violation $violations $repoPath $lineNumber $include "LoFiBoxApp composition must use dedicated page-model/routing/rendering helpers instead of owning input mapping, page rendering, or UI constants"
            }
            if (Test-AnyPrefix $include @("app/app_command_executor.h", "app/app_input_router.h", "app/app_lifecycle.h", "app/app_page_model.h", "app/app_renderer.h", "app/app_state.h", "app/library_controller.h", "app/navigation_state.h", "app/playback_controller.h")) {
                Add-Violation $violations $repoPath $lineNumber $include "LoFiBoxApp must stay a thin public facade; runtime state, controllers, routing, rendering, lifecycle, and command execution belong to AppRuntimeContext"
            }
        }

        if ($repoPath -eq "src/app/app_runtime_context.h") {
            if (Test-AnyPrefix $include @("app/app_state.h", "app/library_controller.h", "app/navigation_state.h", "app/playback_controller.h")) {
                Add-Violation $violations $repoPath $lineNumber $include "AppRuntimeContext must depend on AppRuntimeState and AppControllerSet instead of directly including raw state/controller owners"
            }
        }

        if ($repoPath.StartsWith("src/platform/host/", [System.StringComparison]::Ordinal)) {
            if (Test-AnyPrefix $include @("ui/pages/", "app/lofibox_app.h")) {
                Add-Violation $violations $repoPath $lineNumber $include "host adapters must not depend on concrete app or pages"
            }
        }

        if ($repoPath.StartsWith("src/platform/device/", [System.StringComparison]::Ordinal) -or $repoPath.StartsWith("src/platform/x11/", [System.StringComparison]::Ordinal)) {
            if (Test-AnyPrefix $include @("ui/pages/", "app/lofibox_app.h", "platform/host/")) {
                Add-Violation $violations $repoPath $lineNumber $include "presentation adapters must not depend on host runtime or concrete app/pages"
            }
        }

        if ($repoPath.StartsWith("src/targets/", [System.StringComparison]::Ordinal)) {
            if (Test-AnyPrefix $include @("ui/pages/")) {
                Add-Violation $violations $repoPath $lineNumber $include "targets must stay thin and must not include page implementations"
            }
        }
    }
}

if ($violations.Count -gt 0) {
    if (-not $Quiet) {
        Write-Error ("Architecture boundary violations:`n" + ($violations -join "`n"))
    }
    exit 1
}

if (-not $Quiet) {
    Write-Output "Architecture boundary check passed."
}
