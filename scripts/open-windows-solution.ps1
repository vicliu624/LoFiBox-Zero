# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Preset = "windows-vs2022-test-debug"
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

& (Join-Path $scriptDir "generate-windows-solution.ps1") -Preset $Preset
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$root = Resolve-Path (Join-Path $scriptDir "..")
$solutionPath = Join-Path $root "build\\test\\LoFiBoxZero.sln"

Start-Process $solutionPath
