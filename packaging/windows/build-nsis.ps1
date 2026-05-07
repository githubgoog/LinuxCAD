# Build a LinuxCAD NSIS installer on Windows.
# Requires NSIS (makensis on PATH).

param(
    [string]$Version = $env:LINUXCAD_VERSION,
    [string]$InstallDir = $env:LINUXCAD_INSTALL_DIR,
    [string]$OutDir = $env:LINUXCAD_PACKAGE_DIR
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path "$PSScriptRoot/../..").Path
if (-not $Version)    { $Version    = "1.0.0" }
if (-not $InstallDir) { $InstallDir = Join-Path $Root "build/_install" }
if (-not $OutDir)     { $OutDir     = Join-Path $Root "build/_packages" }

if (-not (Test-Path $InstallDir)) {
    Write-Error "Install dir not found at $InstallDir. Run build-win.ps1 -Install first."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$NsiFile = Join-Path $OutDir "linuxcad.nsi"
@"
!include "MUI2.nsh"

Name "LinuxCAD"
OutFile "LinuxCAD-$Version-Setup.exe"
InstallDir "$PROGRAMFILES64\LinuxCAD"
RequestExecutionLevel admin

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

Section "LinuxCAD"
  SetOutPath "$INSTDIR"
  File /r "$InstallDir\*.*"

  CreateShortcut "$SMPROGRAMS\LinuxCAD.lnk" "$INSTDIR\bin\LinuxCAD.exe"
  CreateShortcut "$DESKTOP\LinuxCAD.lnk"    "$INSTDIR\bin\LinuxCAD.exe"

  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR"
  Delete   "$SMPROGRAMS\LinuxCAD.lnk"
  Delete   "$DESKTOP\LinuxCAD.lnk"
SectionEnd
"@ | Out-File -FilePath $NsiFile -Encoding ASCII

Push-Location $OutDir
try {
    makensis $NsiFile
}
finally {
    Pop-Location
}
Write-Host "Built $OutDir\LinuxCAD-$Version-Setup.exe"
