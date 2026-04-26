# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Preset = "windows-vs2022-test-debug"
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..")

if ($env:OS -ne "Windows_NT") {
    throw "This script is intended for Windows only."
}

cmake --preset $Preset
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$solutionPath = Join-Path $root "build\\test\\LoFiBoxZero.sln"

if (-not (Test-Path $solutionPath)) {
    throw "Expected Visual Studio solution was not generated: $solutionPath"
}

Write-Output "Generated Visual Studio solution:"
Write-Output $solutionPath
