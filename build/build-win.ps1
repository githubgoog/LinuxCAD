# Build LinuxCAD on Windows. Wraps FreeCAD's CMake.
#
# Usage:
#   pwsh build/build-win.ps1
#   pwsh build/build-win.ps1 -Install
#   pwsh build/build-win.ps1 -Reconfigure

param(
    [switch]$Install,
    [switch]$Reconfigure,
    [string]$BuildType = "Release",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$LibPack = ""
)

$ErrorActionPreference = "Stop"

$RootDir    = (Resolve-Path "$PSScriptRoot/..").Path
$SourceDir  = Join-Path $RootDir "FreeCAD-main"
$BuildDir   = if ($env:LINUXCAD_BUILD_DIR)   { $env:LINUXCAD_BUILD_DIR }   else { Join-Path $RootDir "build/_out" }
$InstallDir = if ($env:LINUXCAD_INSTALL_DIR) { $env:LINUXCAD_INSTALL_DIR } else { Join-Path $RootDir "build/_install" }
$Version    = if ($env:LINUXCAD_VERSION)     { $env:LINUXCAD_VERSION }     else { "1.0.0" }

if ($Reconfigure -and (Test-Path $BuildDir)) {
    Remove-Item -Recurse -Force $BuildDir
}

if (-not (Test-Path "$SourceDir/src/3rdParty/GSL/include")) {
    Push-Location $SourceDir
    git submodule update --init --recursive --recommend-shallow
    Pop-Location
}

# Apply LinuxCAD branding (if assets are present in branding/icons/).
$brandingScript = Join-Path $RootDir "branding/apply-branding.sh"
if (Test-Path $brandingScript) {
    if (Get-Command bash -ErrorAction SilentlyContinue) {
        bash $brandingScript
    }
    else {
        Write-Host "branding: bash not found; skipping branding step" -ForegroundColor Yellow
    }
}

New-Item -ItemType Directory -Force -Path $BuildDir,$InstallDir | Out-Null

$cmakeArgs = @(
    "-S", $SourceDir,
    "-B", $BuildDir,
    "-G", $Generator,
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_INSTALL_PREFIX=$InstallDir",
    "-DBUILD_QT5=OFF",
    "-DBUILD_GUI=ON",
    "-DBUILD_FEM_NETGEN=OFF",
    "-DBUILD_VR=OFF",
    "-DBUILD_TEST_RUNNER=OFF",
    "-DENABLE_DEVELOPER_TESTS=OFF",
    "-DPACKAGE_VERSION_SUFFIX=-linuxcad"
)

if ($LibPack) {
    $cmakeArgs += "-DFREECAD_LIBPACK_DIR=$LibPack"
    $cmakeArgs += "-DFREECAD_LIBPACK_USE=ON"
}

Write-Host "Configuring LinuxCAD $Version for Windows..."
cmake @cmakeArgs

Write-Host "Building..."
cmake --build $BuildDir --config $BuildType -- /m

if ($Install) {
    cmake --install $BuildDir --config $BuildType
}
