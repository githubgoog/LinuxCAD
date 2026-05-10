# Build "LinuxCAD for Windows" — a portable .zip bundle produced from the
# pixi-installed tree. The conda env on Windows splits into two roots:
#   <prefix>/python.exe, python*.dll, Lib/, DLLs/   (Python interpreter + stdlib + site-packages)
#   <prefix>/Library/bin/, lib/, share/, Mod/, ... (compiled artefacts; FreeCAD lives here)
# We stage from BOTH so the zip can actually run (FreeCAD on Windows imports
# Python at startup), then prune compile-time tooling so the zip fits under
# GitHub's 2 GB-per-asset cap and ships only runtime payload.
#
# Inputs (all optional):
#   LINUXCAD_INSTALL_DIR  pixi env prefix (default .pixi/envs/default)
#   LINUXCAD_PACKAGE_DIR  output dir (default dist)
#   LINUXCAD_VERSION      version baked into the filename
#   LINUXCAD_ARCH         x64 (default)
#
# Output: dist/LinuxCAD-for-Windows-<version>-<arch>.zip with
#   bin/FreeCAD.exe, bin/LinuxCAD.bat, bin/*.dll  (engine + runtime)
#   python.exe, python*.dll, Lib/, DLLs/         (Python runtime)
#   share/, Mod/, Ext/, ...                       (FreeCAD data)

param(
    [string]$Version    = $env:LINUXCAD_VERSION,
    [string]$EnvRoot    = $env:LINUXCAD_INSTALL_DIR,
    [string]$OutDir     = $env:LINUXCAD_PACKAGE_DIR,
    [string]$Arch       = $env:LINUXCAD_ARCH
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path "$PSScriptRoot/../..").Path

if (-not $Version) { $Version = "1.0.0" }
if (-not $EnvRoot) { $EnvRoot = Join-Path $Root ".pixi/envs/default" }
if (-not $OutDir)  { $OutDir  = Join-Path $Root "dist" }
if (-not $Arch)    { $Arch    = "x64" }

# Back-compat: previous versions of this script and other tooling pointed
# LINUXCAD_INSTALL_DIR at the conda Library subdir. Detect that and step up.
if (-not (Test-Path (Join-Path $EnvRoot "Library"))) {
    $maybeParent = Split-Path -Parent $EnvRoot
    if ($maybeParent -and (Test-Path (Join-Path $maybeParent "Library"))) {
        Write-Host "Note: LINUXCAD_INSTALL_DIR pointed at a Library subdir; using parent $maybeParent as env root."
        $EnvRoot = $maybeParent
    }
}

$Library = Join-Path $EnvRoot "Library"
$FreeCADExe = Join-Path $Library "bin/FreeCAD.exe"
if (-not (Test-Path $FreeCADExe)) {
    Write-Error "FreeCAD.exe not found at $FreeCADExe. Run 'pixi run linuxcad-release' first."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Stage = Join-Path $OutDir "LinuxCAD-stage"
if (Test-Path $Stage) { Remove-Item -Recurse -Force $Stage }
New-Item -ItemType Directory -Force -Path $Stage | Out-Null

# ---- stage Library tree (compiled artefacts: FreeCAD, Qt, etc.) ----
Write-Host "Staging Library tree from $Library"
Copy-Item -Recurse -Force "$Library/*" $Stage

# ---- stage Python runtime from $EnvRoot root ----
# FreeCAD.exe on Windows links against pythonXY.dll and imports stdlib +
# PySide6 from $EnvRoot/Lib at startup; without these the launcher segfaults
# or fails to load the GUI at all. Conda's Windows layout keeps these one
# level above Library/, so the previous "Copy-Item Library/*" approach
# missed them entirely.
Write-Host "Staging Python runtime from $EnvRoot"
foreach ($pat in @("python*.exe","pythonw.exe")) {
    Get-ChildItem -Path $EnvRoot -Filter $pat -File -ErrorAction SilentlyContinue |
        ForEach-Object { Copy-Item -Force $_.FullName -Destination $Stage }
}
# Duplicate python*.dll into both $Stage (next to python.exe at root) and
# $Stage/bin (next to FreeCAD.exe). Windows DLL search resolves from the
# EXE's directory first, so FreeCAD.exe in bin/ would not see a DLL placed
# only at root, and vice versa. The duplication costs ~5 MB; small price for
# both invocation modes (python.exe directly, or via FreeCAD/FreeCADCmd) to
# work without PATH manipulation in the launcher.
$BinForPyDll = Join-Path $Stage "bin"
foreach ($dll in (Get-ChildItem -Path $EnvRoot -Filter "python*.dll" -File -ErrorAction SilentlyContinue)) {
    Copy-Item -Force $dll.FullName -Destination $Stage
    Copy-Item -Force $dll.FullName -Destination $BinForPyDll
}
foreach ($subdir in @("Lib","DLLs")) {
    $src = Join-Path $EnvRoot $subdir
    if (Test-Path $src) {
        Copy-Item -Recurse -Force $src $Stage
    }
}

# ---- prune compile-time bloat ----
# Without this, the Library tree alone is ~9 GB uncompressed (~2.3 GB zipped),
# blowing past GitHub's 2 GB-per-asset release limit. None of these are loaded
# by FreeCAD at runtime — they're build-time tooling that conda-forge installs
# alongside the runtime libraries.

# Headers — only needed when compiling C++ against the libraries.
$Inc = Join-Path $Stage "include"
if (Test-Path $Inc) { Remove-Item -Recurse -Force $Inc }

# mingw64/ — alternative GCC toolchain conda-forge ships for some packages.
$Mingw = Join-Path $Stage "mingw64"
if (Test-Path $Mingw) { Remove-Item -Recurse -Force $Mingw }

# lib/*.lib — Windows import libraries, pure compile-time. Runtime DLLs are
# in bin/ and stay. Note: cmake/, pkg-config files in lib/ are also build-only
# but small enough to leave alone for now.
$LibDir = Join-Path $Stage "lib"
if (Test-Path $LibDir) {
    Get-ChildItem -Path $LibDir -Filter "*.lib" -File -Recurse -ErrorAction SilentlyContinue |
        Remove-Item -Force
}

# bin/ build-time toolchains. clang has 5 hard-link copies in conda-forge
# (clang, clang-cpp, clang-cl, clang-NN, clang++-NN) at ~133 MB each.
$BinDenylist = @(
    # LLVM/Clang/Flang
    "clang.exe","clang++.exe","clang-cl.exe","clang-cpp.exe","clang-[0-9]*.exe","clang++-[0-9]*.exe",
    "libclang*.dll","clang-tidy.exe","clang-format.exe",
    "lld.exe","lld-link.exe","ld.lld.exe","ld64.lld.exe","wasm-ld.exe",
    "flang.exe","flang-new.exe",
    "llvm-*.exe","llc.exe","opt.exe","bbc.exe","FileCheck.exe",
    # Qt build tools (only needed at compile time; runtime Qt is in *.dll)
    "moc.exe","uic.exe","rcc.exe","qmake.exe","qmlcachegen.exe","qmllint.exe",
    "qmlformat.exe","qmltyperegistrar.exe","qsb.exe","balsam*.exe",
    "androiddeployqt6.exe","android_deploy.py","qtpaths*.exe","qtdiag*.exe",
    # Codec / image-format CLI tools (runtime DLLs stay in bin/)
    "aomdec.exe","aomenc.exe",
    # Graphviz CLI tools (runtime libgvc.dll stays)
    "4channels.exe","acyclic.exe","bcomps.exe","ccomps.exe","circo.exe",
    "dot.exe","dot2gxl.exe","dot_builtins.exe","fdp.exe","gc.exe","gml2gv.exe",
    "graphml2gv.exe","gv2gml.exe","gv2gxl.exe","gvcolor.exe","gvgen.exe",
    "gvmap*.exe","gvpack.exe","gvpr.exe","gxl2dot.exe","gxl2gv.exe",
    "neato.exe","nop.exe","osage.exe","patchwork.exe","prune.exe","sccmap.exe",
    "sfdp.exe","tred.exe","twopi.exe","unflatten.exe",
    # Misc shell/env helpers conda pulls in transitively
    "git-bash.exe","git-cmd.exe","sh.exe","bash.exe","busybox.exe"
)
$BinDir = Join-Path $Stage "bin"
foreach ($pat in $BinDenylist) {
    Get-ChildItem -Path $BinDir -Filter $pat -File -ErrorAction SilentlyContinue |
        Remove-Item -Force
}

# Other unrelated subtrees conda pulls in transitively.
foreach ($d in @("java","cmake","doc","man","share/man","share/doc","share/info","mingw-w64")) {
    $p = Join-Path $Stage $d
    if (Test-Path $p) { Remove-Item -Recurse -Force $p }
}

# ---- LinuxCAD launcher .bat ----
# Keeps engine binary intact, exposes LinuxCAD as the user-visible entry point.
# Users double-click bin\LinuxCAD.bat or its desktop shortcut.
$Bat = @'
@echo off
setlocal
set "HERE=%~dp0"
if "%FREECAD_USER_HOME%"=="" set "FREECAD_USER_HOME=%LOCALAPPDATA%\LinuxCAD"
"%HERE%FreeCAD.exe" %*
'@
Set-Content -Path (Join-Path $Stage "bin/LinuxCAD.bat") -Value $Bat -Encoding ASCII

# ---- compress ----
$Zip = Join-Path $OutDir "LinuxCAD-for-Windows-$Version-$Arch.zip"
if (Test-Path $Zip) { Remove-Item -Force $Zip }
Write-Host "Compressing $Stage -> $Zip"
Compress-Archive -Path (Join-Path $Stage '*') -DestinationPath $Zip -CompressionLevel Optimal

$ZipSize = (Get-Item $Zip).Length
$ZipSizeMB = [math]::Round($ZipSize / 1MB, 1)
Write-Host "Built $Zip ($ZipSizeMB MB)"

# 2 GiB = 2 * 1024^3 = 2147483648 bytes. GitHub Releases rejects assets above this.
$GhAssetLimit = 2147483648
if ($ZipSize -ge $GhAssetLimit) {
    Write-Warning "Zip is $ZipSizeMB MB; GitHub Releases rejects assets >= 2048 MiB."
    Write-Warning "Add more entries to the bin/ denylist or expand the prune list in build-zip.ps1."
}

Remove-Item -Recurse -Force $Stage
