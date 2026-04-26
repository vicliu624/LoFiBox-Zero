# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Image = "lofibox-zero/dev-container:trixie"
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$script = Join-Path $repo ".tmp\check-debian-builddeps.sh"

New-Item -ItemType Directory -Force -Path (Split-Path $script) | Out-Null

docker image inspect $Image *> $null
if ($LASTEXITCODE -ne 0) {
    throw "Required local Docker image '$Image' was not found. Build or import it explicitly before running this script."
}

$content = @'
set -euo pipefail

command -v dpkg-buildpackage

output="$(dpkg-checkbuilddeps debian/control 2>&1 || true)"
if [ -n "$output" ]; then
  echo "$output" >&2
  exit 1
fi
'@

$content = $content -replace "`r`n", "`n"
[System.IO.File]::WriteAllText($script, $content, [System.Text.UTF8Encoding]::new($false))

docker run --rm `
    -v "${repo}:/workspace/LoFiBox-Zero" `
    -w /workspace/LoFiBox-Zero `
    $Image `
    bash /workspace/LoFiBox-Zero/.tmp/check-debian-builddeps.sh

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
