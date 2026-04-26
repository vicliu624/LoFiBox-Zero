# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Image = "lofibox-zero/package-build:trixie",
    [string]$Version = "0.1.0"
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$parent = Split-Path $repo
$archive = Join-Path $parent "lofibox_$Version.orig.tar.xz"

docker image inspect $Image *> $null
if ($LASTEXITCODE -ne 0) {
    throw "Required local Docker image '$Image' was not found. Run scripts/build-debian-package-image.ps1 first."
}

if (Test-Path $archive) {
    Remove-Item -LiteralPath $archive -Force
}

$script = Join-Path $repo ".tmp\create-orig-tarball.sh"
New-Item -ItemType Directory -Force -Path (Split-Path $script) | Out-Null

$content = @'
set -euxo pipefail

version="$1"
archive="/workspace/lofibox_${version}.orig.tar.xz"
prefix="lofibox-${version}"

tar \
  --exclude='./.git' \
  --exclude='./.cache' \
  --exclude='./.tmp' \
  --exclude='./build' \
  --exclude='./out' \
  --exclude='./obj-*' \
  --exclude='./runs' \
  --exclude='./debian' \
  --exclude='./*.deb' \
  --exclude='./*.buildinfo' \
  --exclude='./*.changes' \
  --exclude='./*.dsc' \
  --exclude='./*.debian.tar.*' \
  --transform "s#^\./#${prefix}/#" \
  -cJf "$archive" .

manifest="$(mktemp)"
tar -tf "$archive" > "$manifest"
grep -q "^${prefix}/CMakeLists.txt$" "$manifest"
grep -q "^${prefix}/src/" "$manifest"
if grep -q "^${prefix}/debian/" "$manifest"; then
  echo "orig tarball must not contain debian/" >&2
  exit 1
fi
'@

$content = $content -replace "`r`n", "`n"
[System.IO.File]::WriteAllText($script, $content, [System.Text.UTF8Encoding]::new($false))

docker run --rm `
    -v "${parent}:/workspace" `
    -w /workspace/LoFiBox-Zero `
    $Image `
    bash /workspace/LoFiBox-Zero/.tmp/create-orig-tarball.sh $Version

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
