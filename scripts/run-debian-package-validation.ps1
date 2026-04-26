# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Image = "lofibox-zero/package-build:trixie",
    [string]$BuildArgs = "-us -uc",
    [switch]$SkipOrigTarball
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$parent = Split-Path $repo
$changes = Join-Path $parent "lofibox_0.1.0-1_amd64.changes"

if (-not $SkipOrigTarball) {
    & (Join-Path $PSScriptRoot "create-orig-tarball.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

& (Join-Path $PSScriptRoot "run-dpkg-buildpackage.ps1") -Image $Image -BuildArgs $BuildArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if (-not (Test-Path -LiteralPath $changes)) {
    throw "Expected Debian changes file was not produced: $changes"
}

docker image inspect $Image *> $null
if ($LASTEXITCODE -ne 0) {
    throw "Required local Docker image '$Image' was not found. Run scripts/build-debian-package-image.ps1 first."
}

docker run --rm `
    -v "${parent}:/workspace" `
    -w /workspace `
    $Image `
    bash -lc "set -euxo pipefail; lintian lofibox_0.1.0-1_amd64.changes; autopkgtest lofibox_0.1.0-1_amd64.changes -- null"

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
