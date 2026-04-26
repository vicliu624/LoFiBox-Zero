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
Assert-PathExists "src/playback/playback_controller.cpp" "Playback controller implementation must live under src/playback."
Assert-PathExists "src/playback/playback_enrichment_coordinator.cpp" "Playback enrichment implementation must live under src/playback."
Assert-PathExists "src/playback/playback_runtime_coordinator.cpp" "Playback runtime implementation must live under src/playback."
Assert-PathExists "src/playback/mixed_playback_queue.cpp" "Mixed playback queue implementation must live under src/playback."

Assert-PathExists "src/audio/decoder/audio_decoder_contract.h" "Audio decoder boundary must live under src/audio/decoder."
Assert-PathExists "src/audio/output/audio_output_contract.h" "Audio output boundary must live under src/audio/output."
Assert-PathExists "src/audio/output/host_audio_playback_backend.cpp" "Host audio output implementation must live under src/audio/output."
Assert-PathExists "src/cache/cache_manager.cpp" "Cache/offline implementation must live under src/cache."
Assert-PathExists "src/cache/cache_manager.h" "Cache/offline boundary must live under src/cache."

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
