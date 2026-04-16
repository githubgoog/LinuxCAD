Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Require-Command([string]$name) {
  if (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
    throw "Required command '$name' was not found in PATH."
  }
}

Require-Command 'cmake'

$rootDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$frontendDir = Join-Path $rootDir 'frontend-qml'

if (-not (Test-Path $frontendDir)) {
  throw "QML frontend folder not found at $frontendDir"
}

$buildDir = Join-Path $frontendDir 'build-debug'
$desktopBinary = Join-Path $buildDir 'linuxcad-qml'

if (-not (Test-Path $desktopBinary)) {
  Write-Host 'Configuring LinuxCAD QML frontend (Debug)...'
  & cmake -S $frontendDir -B $buildDir -DCMAKE_BUILD_TYPE=Debug
  Write-Host 'Building LinuxCAD QML frontend (Debug)...'
  & cmake --build $buildDir
}

Write-Host 'Launching LinuxCAD QML desktop app...'
& $desktopBinary
