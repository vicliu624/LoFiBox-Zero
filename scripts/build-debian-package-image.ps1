# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Image = "lofibox-zero/package-build:trixie",
    [string]$BaseImage = "lofibox-zero/dev-container:trixie"
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

docker image inspect $BaseImage *> $null
if ($LASTEXITCODE -ne 0) {
    throw "Required local base image '$BaseImage' was not found. Build or import it explicitly before running this script."
}

docker build `
    --build-arg "BASE_IMAGE=$BaseImage" `
    -f docker/package-build.Dockerfile `
    -t $Image `
    $repo

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
