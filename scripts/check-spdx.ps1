param(
    [switch]$Quiet
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

$excludedPathParts = @(
    ".git",
    ".cache",
    ".tmp",
    "build",
    "out",
    "runs",
    "obj-x86_64-linux-gnu",
    "src/third_party"
)

$extensions = @(
    ".c",
    ".cc",
    ".cpp",
    ".h",
    ".hpp",
    ".cmake",
    ".md",
    ".py",
    ".ps1",
    ".sh",
    ".yml",
    ".yaml",
    ".desktop",
    ".xml",
    ".svg",
    ".1"
)

$specialNames = @("CMakeLists.txt")

function Convert-ToRepoPath([string]$Path)
{
    return $Path.Substring($repo.Length + 1).Replace("\", "/")
}

function Is-Excluded([string]$RepoPath)
{
    foreach ($part in $excludedPathParts) {
        if ($RepoPath -eq $part -or $RepoPath.StartsWith("$part/", [System.StringComparison]::Ordinal)) {
            return $true
        }
    }
    return $false
}

$missing = @()

Get-ChildItem -Path $repo -Recurse -File | ForEach-Object {
    $repoPath = Convert-ToRepoPath $_.FullName
    if (Is-Excluded $repoPath) {
        return
    }

    $extension = $_.Extension
    if (-not ($extensions -contains $extension) -and -not ($specialNames -contains $_.Name)) {
        return
    }

    if (-not (Select-String -Path $_.FullName -Pattern "SPDX-License-Identifier" -Quiet)) {
        $missing += $repoPath
    }
}

if ($missing.Count -gt 0) {
    if (-not $Quiet) {
        Write-Error ("Missing SPDX-License-Identifier in:`n" + ($missing -join "`n"))
    }
    exit 1
}

if (-not $Quiet) {
    Write-Output "SPDX check passed."
}
