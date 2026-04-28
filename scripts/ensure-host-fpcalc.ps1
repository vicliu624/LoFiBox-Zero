# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [switch]$Install,
    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$HostPlatform = [System.Environment]::OSVersion.Platform
$IsWindowsHost = $HostPlatform -eq [System.PlatformID]::Win32NT
$IsLinuxHost = -not $IsWindowsHost -and (Test-Path "/etc/os-release")

function Write-Status {
    param([string]$Message)
    if (-not $Quiet) {
        Write-Host $Message
    }
}

function Get-ExecutableCommand {
    param([string]$Name)
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        return $null
    }
    return $command.Source
}

function Test-Fpcalc {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $false
    }
    try {
        $version = & $Path -version 2>$null
        if ($LASTEXITCODE -eq 0 -or $version) {
            if (-not $Quiet) {
                Write-Host "fpcalc available: $Path"
                if ($version) {
                    Write-Host ($version | Select-Object -First 1)
                }
            }
            return $true
        }
    } catch {
        return $false
    }
    return $false
}

function Install-Fpcalc {
    if ($IsWindowsHost) {
        if (Get-ExecutableCommand "scoop") {
            Write-Status "Installing Chromaprint/fpcalc with scoop."
            & scoop install chromaprint
            return
        }
        if (Get-ExecutableCommand "winget") {
            Write-Status "Installing Chromaprint/fpcalc with winget."
            & winget install --exact --id AcoustID.Chromaprint --accept-source-agreements --accept-package-agreements
            return
        }
        if (Get-ExecutableCommand "choco") {
            Write-Status "Installing Chromaprint/fpcalc with Chocolatey."
            & choco install chromaprint -y
            return
        }
        throw "No supported Windows package manager found. Install Chromaprint/fpcalc manually and set FPCALC_PATH."
    }

    if ($IsLinuxHost) {
        if (Get-ExecutableCommand "apt-get") {
            Write-Status "Installing libchromaprint-tools with apt-get."
            & sudo apt-get update
            & sudo apt-get install -y libchromaprint-tools
            return
        }
        if (Get-ExecutableCommand "dnf") {
            Write-Status "Installing chromaprint-tools with dnf."
            & sudo dnf install -y chromaprint-tools
            return
        }
        if (Get-ExecutableCommand "pacman") {
            Write-Status "Installing chromaprint with pacman."
            & sudo pacman -S --noconfirm chromaprint
            return
        }
    }

    if (Get-ExecutableCommand "brew") {
        Write-Status "Installing chromaprint with Homebrew."
        & brew install chromaprint
        return
    }

    throw "No supported package manager found. Install Chromaprint/fpcalc manually and set FPCALC_PATH."
}

$explicit = $env:FPCALC_PATH
if (-not [string]::IsNullOrWhiteSpace($explicit) -and (Test-Fpcalc $explicit)) {
    exit 0
}

$pathCandidate = Get-ExecutableCommand "fpcalc"
if ($null -ne $pathCandidate -and (Test-Fpcalc $pathCandidate)) {
    exit 0
}

if ($Install) {
    Install-Fpcalc
    $pathCandidate = Get-ExecutableCommand "fpcalc"
    if ($null -ne $pathCandidate -and (Test-Fpcalc $pathCandidate)) {
        exit 0
    }
}

Write-Error "fpcalc is unavailable. Run this script with -Install, install Chromaprint/fpcalc manually, or set FPCALC_PATH."
exit 1
