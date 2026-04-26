# SPDX-License-Identifier: GPL-3.0-or-later

[CmdletBinding()]
param(
    [string]$Distro = "Ubuntu-24.04",
    [ValidateSet("build", "device", "shell")]
    [string]$Mode = "build",
    [string]$BuildType = "Debug",
    [string]$Platform,
    [string]$BaseImage,
    [string]$MediaRoot,
    [string]$ImageTag,
    [switch]$SkipImageBuild,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$DeviceArgs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Convert-ToWslPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$WindowsPath
    )

    $resolvedPath = (Resolve-Path -LiteralPath $WindowsPath).Path
    $driveName = $resolvedPath.Substring(0, 1).ToLowerInvariant()
    $restPath = $resolvedPath.Substring(2).Replace('\', '/')
    return "/mnt/$driveName$restPath"
}

function Resolve-MediaRoot {
    param(
        [string]$ExplicitMediaRoot
    )

    if ($ExplicitMediaRoot) {
        return (Resolve-Path -LiteralPath $ExplicitMediaRoot).Path
    }

    foreach ($scope in @("Process", "User", "Machine")) {
        $candidate = [Environment]::GetEnvironmentVariable("LOFIBOX_MEDIA_ROOT", $scope)
        if ($candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

$repositoryRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")
$repositoryRootWsl = Convert-ToWslPath -WindowsPath $repositoryRoot
$resolvedMediaRoot = Resolve-MediaRoot -ExplicitMediaRoot $MediaRoot

$wslArguments = @(
    "-d", $Distro,
    "--cd", $repositoryRootWsl,
    "env",
    "DEV_CONTAINER_MODE=$Mode",
    "DEV_CONTAINER_BUILD_TYPE=$BuildType"
)

if ($Platform) {
    $wslArguments += "DEV_CONTAINER_PLATFORM=$Platform"
}

if ($BaseImage) {
    $wslArguments += "DEV_CONTAINER_BASE_IMAGE=$BaseImage"
}

if ($ImageTag) {
    $wslArguments += "DEV_CONTAINER_IMAGE=$ImageTag"
}

if ($resolvedMediaRoot) {
    $wslArguments += "DEV_CONTAINER_MEDIA_ROOT_HOST=$(Convert-ToWslPath -WindowsPath $resolvedMediaRoot)"
}

if ($SkipImageBuild.IsPresent) {
    $wslArguments += "DEV_CONTAINER_SKIP_BUILD=1"
}

$wslArguments += "bash"
$wslArguments += "scripts/run-dev-container.sh"

if ($DeviceArgs -and $DeviceArgs.Count -gt 0) {
    $wslArguments += "--"
    $wslArguments += $DeviceArgs
}

& wsl.exe @wslArguments
exit $LASTEXITCODE
