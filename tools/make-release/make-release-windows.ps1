<#
.SYNOPSIS
    Assemble a RetroDebugger Windows release.
.DESCRIPTION
    Five steps: stage the release tree (README, docs, tools; no icons),
    clean-build via build-windows.ps1, copy the .exe, then either zip it
    (default) or leave the folder for CI to upload (-NoZip).

    Output (default):  releases\RetroDebugger-v<version>-windows-<arch>.zip
    Output (-NoZip):   releases\RetroDebugger-v<version>\   (folder)

    build-windows.ps1 assumes MTEngineSDL + uSockets exist as sibling
    checkouts, so this script clones them if missing (mirrors build-linux.sh).
.PARAMETER NoZip
    Do not create a .zip. Assemble the release FOLDER in releases\ instead.
    Intended for CI: GitHub Actions zips the uploaded folder once, avoiding a
    pointless pack -> unpack -> repack round-trip. Under GitHub Actions this
    also sets the step outputs 'release_dir' and 'artifact_name'.
#>
[CmdletBinding()]
param(
    [switch]$NoZip
)

$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = (Resolve-Path (Join-Path $ScriptDir '..\..')).Path
Set-Location $Root

# --- version ---
$verFile = Join-Path $Root 'src\C64D_Version.h'
$m = Select-String -Path $verFile -Pattern 'RETRODEBUGGER_VERSION_STRING\s*"([^"]*)"'
if (-not $m) { throw "Could not parse RETRODEBUGGER_VERSION_STRING from $verFile" }
$Version = $m.Matches[0].Groups[1].Value

# --- arch (uniform lowercase in names) ---
$Platform = if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') { 'ARM64' } else { 'x64' }
$Arch = $Platform.ToLower()

$ReleaseName  = "RetroDebugger-v$Version"
$ArtifactName = "RetroDebugger-v$Version-windows-$Arch"
$ZipName      = "$ArtifactName.zip"

# --- ensure sibling deps exist (build-windows.ps1 assumes they do) ---
$Parent = Split-Path -Parent $Root
$MtDir = Join-Path $Parent 'MTEngineSDL'
if (-not (Test-Path $MtDir)) {
    Write-Host "==> Cloning MTEngineSDL"
    git clone --recursive https://github.com/slajerek/MTEngineSDL.git $MtDir
    if ($LASTEXITCODE -ne 0) { throw "git clone MTEngineSDL failed" }
}
$UsDir = Join-Path $Parent 'uSockets'
if (-not (Test-Path $UsDir)) {
    Write-Host "==> Cloning uSockets"
    git clone https://github.com/uNetworking/uSockets.git $UsDir
    if ($LASTEXITCODE -ne 0) { throw "git clone uSockets failed" }
}

Write-Host "==> [1/5] Staging $ReleaseName (windows-$Arch)"
$Stage = Join-Path ([System.IO.Path]::GetTempPath()) ("rd-rel-" + [System.Guid]::NewGuid().ToString('N'))
$ReleaseDir = Join-Path $Stage $ReleaseName
New-Item -ItemType Directory -Force -Path $ReleaseDir | Out-Null

Copy-Item (Join-Path $Root 'README.md') $ReleaseDir
$DocsDir = Join-Path $ReleaseDir 'docs'
New-Item -ItemType Directory -Force -Path $DocsDir | Out-Null
Copy-Item (Join-Path $Root 'docs\README-C64-65XE-NES-Debugger.txt') $DocsDir
Copy-Item (Join-Path $Root 'docs\release-notes.txt') $DocsDir
$ToolsDir = Join-Path $ReleaseDir 'tools'
New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null
Copy-Item (Join-Path $Root 'tools\c64d-champ') $ToolsDir -Recurse
Copy-Item (Join-Path $Root 'tools\websockets-debugger-test') $ToolsDir -Recurse
# Never ship node_modules in releases.
Get-ChildItem $ToolsDir -Recurse -Directory -Filter node_modules -ErrorAction SilentlyContinue |
    Remove-Item -Recurse -Force

Write-Host "==> [2/5] Building RetroDebugger (clean)"
& (Join-Path $Root 'build-windows.ps1') -Platform $Platform -Configuration Release -Clean
& (Join-Path $Root 'build-windows.ps1') -Platform $Platform -Configuration Release
if ($LASTEXITCODE -ne 0) { throw "build-windows.ps1 failed with exit code $LASTEXITCODE" }

Write-Host "==> [3/5] Copying binary"
$OutDir = Join-Path $Root "platform\Windows\bin\$Platform\Release"
$Exe = Join-Path $OutDir 'Retro Debugger.exe'
if (-not (Test-Path $Exe)) { $Exe = Join-Path $OutDir 'retrodebugger.exe' }
if (-not (Test-Path $Exe)) { $Exe = Join-Path $OutDir 'c64d.exe' }
if (-not (Test-Path $Exe)) { throw "Built executable not found in $OutDir" }
Copy-Item $Exe $ReleaseDir

$ReleasesDir = Join-Path $Root 'releases'
New-Item -ItemType Directory -Force -Path $ReleasesDir | Out-Null

if ($NoZip) {
    Write-Host "==> [4/5] Collecting release folder (no zip)"
    $Dest = Join-Path $ReleasesDir $ReleaseName
    if (Test-Path $Dest) { Remove-Item $Dest -Recurse -Force }
    Move-Item $ReleaseDir $Dest
    Remove-Item $Stage -Recurse -Force -ErrorAction SilentlyContinue

    Write-Host "==> [5/5] Release folder ready: $Dest"
    Write-Host "==> Artifact name: $ArtifactName"
    if ($env:GITHUB_OUTPUT) {
        "release_dir=releases/$ReleaseName" >> $env:GITHUB_OUTPUT
        "artifact_name=$ArtifactName"       >> $env:GITHUB_OUTPUT
    }
} else {
    Write-Host "==> [4/5] Packaging $ZipName"
    $Zip = Join-Path $ReleasesDir $ZipName
    if (Test-Path $Zip) { Remove-Item $Zip -Force }
    Compress-Archive -Path $ReleaseDir -DestinationPath $Zip
    Remove-Item $Stage -Recurse -Force -ErrorAction SilentlyContinue

    Write-Host "==> [5/5] Release ready: $Zip"
}
