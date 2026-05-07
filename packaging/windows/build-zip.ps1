# Build "LinuxCAD for Windows" — a portable .zip bundle produced from the
# pixi-installed tree (.pixi/envs/default/Library on Windows).
#
# Inputs (all optional):
#   LINUXCAD_INSTALL_DIR  pixi env prefix (default .pixi/envs/default/Library)
#   LINUXCAD_PACKAGE_DIR  output dir (default dist)
#   LINUXCAD_VERSION      version baked into the filename
#   LINUXCAD_ARCH         x64 (default)
#
# Output: dist/LinuxCAD-for-Windows-<version>-<arch>.zip with
#   bin/FreeCAD.exe        (engine)
#   bin/LinuxCAD.bat       (user-facing entry point that launches FreeCAD.exe)
#   lib, share, ...        (rest of the conda env)

param(
    [string]$Version    = $env:LINUXCAD_VERSION,
    [string]$InstallDir = $env:LINUXCAD_INSTALL_DIR,
    [string]$OutDir     = $env:LINUXCAD_PACKAGE_DIR,
    [string]$Arch       = $env:LINUXCAD_ARCH
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path "$PSScriptRoot/../..").Path

if (-not $Version)    { $Version    = "1.0.0" }
if (-not $InstallDir) { $InstallDir = Join-Path $Root ".pixi/envs/default/Library" }
if (-not $OutDir)     { $OutDir     = Join-Path $Root "dist" }
if (-not $Arch)       { $Arch       = "x64" }

if (-not (Test-Path (Join-Path $InstallDir "bin/FreeCAD.exe"))) {
    Write-Error "FreeCAD.exe not found under $InstallDir. Run 'pixi run linuxcad-release' first."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Stage = Join-Path $OutDir "LinuxCAD-stage"
if (Test-Path $Stage) { Remove-Item -Recurse -Force $Stage }
New-Item -ItemType Directory -Force -Path $Stage | Out-Null

Copy-Item -Recurse -Force "$InstallDir/*" $Stage

# LinuxCAD launcher .bat — keeps engine binary intact, exposes LinuxCAD as the
# user-visible entry point. Users double-click bin\LinuxCAD.bat or shortcut.
$Bat = @'
@echo off
setlocal
set "HERE=%~dp0"
if "%FREECAD_USER_HOME%"=="" set "FREECAD_USER_HOME=%LOCALAPPDATA%\LinuxCAD"
"%HERE%FreeCAD.exe" %*
'@
Set-Content -Path (Join-Path $Stage "bin/LinuxCAD.bat") -Value $Bat -Encoding ASCII

$Zip = Join-Path $OutDir "LinuxCAD-for-Windows-$Version-$Arch.zip"
if (Test-Path $Zip) { Remove-Item -Force $Zip }
Compress-Archive -Path (Join-Path $Stage '*') -DestinationPath $Zip -CompressionLevel Optimal

Remove-Item -Recurse -Force $Stage
Write-Host "Built $Zip"
