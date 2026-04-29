# SPDX-License-Identifier: GPL-3.0-or-later

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

$errors = New-Object System.Collections.Generic.List[string]

function Assert-PathExists($path, $message) {
    if (-not (Test-Path $path)) {
        $errors.Add($message)
    }
}

function Assert-NoPath($path, $message) {
    if (Test-Path $path) {
        $errors.Add($message)
    }
}

function Assert-FileDoesNotContain($path, $pattern, $message) {
    if ((Test-Path $path) -and (Select-String -Path $path -Pattern $pattern -Quiet)) {
        $errors.Add($message)
    }
}

$srcGitkeep = Get-ChildItem -Path "src" -Recurse -Force -File -Filter ".gitkeep" -ErrorAction SilentlyContinue
foreach ($item in $srcGitkeep) {
    $errors.Add("src must not use .gitkeep implementation placeholders: $($item.FullName)")
}

$pycacheDirs = Get-ChildItem -Path "src" -Recurse -Force -Directory -Filter "__pycache__" -ErrorAction SilentlyContinue
foreach ($dir in $pycacheDirs) {
    $errors.Add("src must not contain Python runtime cache directories: $($dir.FullName)")
}

$emptySrcDirs = Get-ChildItem -Path "src" -Recurse -Directory -Force | Where-Object {
    -not (Get-ChildItem -Path $_.FullName -Force -File -ErrorAction SilentlyContinue)
}
foreach ($dir in $emptySrcDirs) {
    $errors.Add("src directory must not be empty or purely aspirational: $($dir.FullName)")
}

Assert-PathExists "src/remote/emby/emby_provider.py" "Emby provider implementation must live under src/remote/emby."
Assert-PathExists "src/remote/jellyfin/jellyfin_provider.py" "Jellyfin provider implementation must live under src/remote/jellyfin."
Assert-PathExists "src/remote/opensubsonic/opensubsonic_provider.py" "OpenSubsonic provider implementation must live under src/remote/opensubsonic."
Assert-PathExists "src/remote/navidrome/navidrome_provider.py" "Navidrome provider implementation must live under src/remote/navidrome."
Assert-PathExists "src/remote/tooling/remote_media_dispatcher.py" "Remote helper dispatcher must live under src/remote/tooling."

Assert-FileDoesNotContain "scripts/remote_media_tool.py" "def _.*(emby|jellyfin|subsonic|navidrome)" "scripts/remote_media_tool.py must be a thin entrypoint, not a protocol implementation."
Assert-FileDoesNotContain "scripts/remote_media_tool.py" "AuthenticateByName|/rest/stream.view|/Audio/" "scripts/remote_media_tool.py must not contain protocol endpoint details."

Assert-NoPath "src/app/playback_controller.cpp" "Playback controller implementation must not live under src/app."
Assert-NoPath "src/app/playback_enrichment_coordinator.cpp" "Playback enrichment implementation must not live under src/app."
Assert-NoPath "src/app/playback_runtime_coordinator.cpp" "Playback runtime implementation must not live under src/app."
Assert-NoPath "src/app/mixed_playback_queue.cpp" "Mixed playback queue implementation must not live under src/app."
Assert-NoPath "src/app/library_query_service.cpp" "Reusable library query service must live under src/application."
Assert-NoPath "src/app/library_mutation_service.cpp" "Reusable library mutation service must live under src/application."
Assert-NoPath "src/app/library_open_action_resolver.cpp" "Reusable library open-action service must live under src/application."
Assert-PathExists "src/playback/playback_controller.cpp" "Playback controller implementation must live under src/playback."
Assert-PathExists "src/playback/playback_enrichment_coordinator.cpp" "Playback enrichment implementation must live under src/playback."
Assert-PathExists "src/playback/playback_runtime_coordinator.cpp" "Playback runtime implementation must live under src/playback."
Assert-PathExists "src/playback/mixed_playback_queue.cpp" "Mixed playback queue implementation must live under src/playback."
Assert-PathExists "src/application/app_service_registry.cpp" "Application service registry must live under src/application."
Assert-PathExists "src/application/playback_command_service.cpp" "Playback command service must live under src/application."
Assert-PathExists "src/application/queue_command_service.cpp" "Queue command service must live under src/application."
Assert-PathExists "src/application/playback_status_query_service.cpp" "Playback status query service must live under src/application."
Assert-PathExists "src/application/library_query_service.cpp" "Library query service must live under src/application."
Assert-PathExists "src/application/library_mutation_service.cpp" "Library mutation service must live under src/application."
Assert-PathExists "src/application/library_open_action_service.cpp" "Library open-action service must live under src/application."
Assert-PathExists "src/application/source_profile_command_service.cpp" "Source profile command service must live under src/application."
Assert-PathExists "src/application/remote_browse_query_service.cpp" "Remote browse query service must live under src/application."
Assert-PathExists "src/application/credential_command_service.cpp" "Credential command service must live under src/application."
Assert-PathExists "src/application/cache_command_service.cpp" "Cache command service must live under src/application."
Assert-PathExists "src/application/runtime_diagnostic_service.cpp" "Runtime diagnostic service must live under src/application."
Assert-PathExists "src/cli/direct_cli.cpp" "Direct CLI dispatcher must live under src/cli."
Assert-PathExists "src/runtime/runtime_command.h" "Runtime command contract must live under src/runtime."
Assert-PathExists "src/runtime/runtime_result.cpp" "Runtime command result implementation must live under src/runtime."
Assert-PathExists "src/runtime/runtime_snapshot.h" "Runtime snapshot contract must live under src/runtime."
Assert-PathExists "src/runtime/runtime_session_facade.cpp" "Runtime session facade must live under src/runtime."
Assert-PathExists "src/runtime/playback_runtime.cpp" "Playback runtime domain must live under src/runtime."
Assert-PathExists "src/runtime/queue_runtime.cpp" "Queue runtime domain must live under src/runtime."
Assert-PathExists "src/runtime/eq_runtime.cpp" "EQ runtime domain must live under src/runtime."
Assert-PathExists "src/runtime/remote_session_runtime.cpp" "Remote session runtime domain must live under src/runtime."
Assert-PathExists "src/runtime/settings_runtime.cpp" "Settings runtime domain must live under src/runtime."
Assert-PathExists "src/runtime/runtime_snapshot_assembler.cpp" "Runtime snapshot assembler must live under src/runtime."
Assert-PathExists "src/runtime/runtime_command_dispatcher.cpp" "Runtime command dispatcher must live under src/runtime."
Assert-PathExists "src/runtime/runtime_query_dispatcher.cpp" "Runtime query dispatcher must live under src/runtime."
Assert-PathExists "src/runtime/runtime_command_bus.cpp" "Runtime command bus must live under src/runtime."
Assert-PathExists "src/runtime/runtime_command_client.h" "Runtime command client contract must live under src/runtime."
Assert-PathExists "src/runtime/runtime_command_server.cpp" "Runtime command server must live under src/runtime."
Assert-PathExists "src/runtime/runtime_transport.h" "Runtime transport-neutral envelope contract must live under src/runtime."
Assert-PathExists "src/runtime/in_process_runtime_client.cpp" "In-process runtime client adapter must live under src/runtime."
Assert-PathExists "src/runtime/runtime_host.cpp" "Runtime host ownership must live under src/runtime."
Assert-PathExists "src/runtime/eq_runtime_state.h" "EQ runtime truth state must live under src/runtime."
Assert-PathExists "src/runtime/settings_runtime_state.h" "Settings runtime truth state must live under src/runtime."
Assert-PathExists "src/runtime/runtime_envelope_serializer.cpp" "Runtime envelope serializer must live under src/runtime."
Assert-PathExists "src/runtime/unix_socket_runtime_transport.cpp" "Unix socket runtime transport must live under src/runtime."
Assert-PathExists "src/app/eq_ui_state.h" "EQ GUI selection state must live under src/app."
Assert-PathExists "src/app/settings_ui_state.h" "Settings GUI selection state must live under src/app."
Assert-PathExists "src/cli/runtime_cli.cpp" "Runtime CLI adapter must live under src/cli."

Assert-PathExists "src/audio/decoder/audio_decoder_contract.h" "Audio decoder boundary must live under src/audio/decoder."
Assert-PathExists "src/audio/output/audio_output_contract.h" "Audio output boundary must live under src/audio/output."
Assert-PathExists "src/audio/output/host_audio_playback_backend.cpp" "Host audio output implementation must live under src/audio/output."
Assert-PathExists "src/cache/cache_manager.cpp" "Cache/offline implementation must live under src/cache."
Assert-PathExists "src/cache/cache_manager.h" "Cache/offline boundary must live under src/cache."

Assert-PathExists "src/remote/common/remote_catalog_model.cpp" "Remote catalog model must live under src/remote/common."
Assert-PathExists "src/remote/common/stream_source_model.cpp" "Stream source classifier must live under src/remote/common."
Assert-PathExists "src/playback/streaming_playback_policy.cpp" "Streaming playback policy must live under src/playback."
Assert-PathExists "src/playback/playback_stability_policy.cpp" "Playback stability policy must live under src/playback."
Assert-PathExists "src/library/library_governance.cpp" "Library governance must live under src/library."
Assert-PathExists "src/metadata/metadata_governance.cpp" "Metadata governance must live under src/metadata."

Assert-PathExists "src/playlist/playlist_parser.cpp" "Playlist parser implementation must live under src/playlist."
Assert-PathExists "src/plugins/plugin_manifest.cpp" "Plugin manifest implementation must live under src/plugins."

$cmake = Get-Content "CMakeLists.txt" -Raw
foreach ($forbidden in @(
    "src/app/playback_controller.cpp",
    "src/app/playback_enrichment_coordinator.cpp",
    "src/app/playback_runtime_coordinator.cpp",
    "src/app/mixed_playback_queue.cpp",
    "src/platform/host/audio_playback_backend.cpp"
)) {
    if ($cmake.Contains($forbidden)) {
        $errors.Add("CMakeLists.txt still builds implementation from forbidden placement: $forbidden")
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output "Implementation placement check passed."
