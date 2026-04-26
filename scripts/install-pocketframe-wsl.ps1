# SPDX-License-Identifier: GPL-3.0-or-later

[CmdletBinding()]
param(
    [string]$Distro = "Ubuntu-24.04"
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

$repositoryRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")
$repositoryRootWsl = Convert-ToWslPath -WindowsPath $repositoryRoot

& wsl.exe -d $Distro --cd $repositoryRootWsl bash scripts/install-pocketframe-wsl.sh
exit $LASTEXITCODE
