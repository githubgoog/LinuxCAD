# LinuxCAD for Windows installer.
#
# Run from PowerShell:
#   iwr -useb https://raw.githubusercontent.com/githubgoog/LinuxCAD/main/install.ps1 | iex
#   # or, locally:
#   powershell -ExecutionPolicy Bypass -File install.ps1
#
# What it does:
#   1. Removes any older LinuxCAD install in %LOCALAPPDATA%\Programs\LinuxCAD.
#   2. Downloads the matching .zip from the latest GitHub Release.
#   3. Expands it to %LOCALAPPDATA%\Programs\LinuxCAD.
#   4. Adds Start menu and Desktop shortcuts pointing at bin\LinuxCAD.bat.

[CmdletBinding()]
param(
    [string]$Repo = $(if ($env:LINUXCAD_REPO) { $env:LINUXCAD_REPO } else { 'githubgoog/LinuxCAD' }),
    [string]$Version,
    [string]$ZipUrl,
    [string]$LocalZip,
    [switch]$Uninstall
)

$ErrorActionPreference = 'Stop'
function Log($msg)  { Write-Host "[linuxcad] $msg" }
function Warn($msg) { Write-Warning "[linuxcad] $msg" }
function Die($msg)  { Write-Error "[linuxcad] error: $msg"; exit 1 }

$InstallDir = Join-Path $env:LOCALAPPDATA 'Programs\LinuxCAD'
$ShortcutSm = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\LinuxCAD.lnk'
$ShortcutDt = Join-Path ([Environment]::GetFolderPath('Desktop')) 'LinuxCAD.lnk'

function Remove-LinuxCAD {
    Log "Removing existing install at $InstallDir..."
    if (Test-Path $InstallDir) { Remove-Item -Recurse -Force $InstallDir }
    if (Test-Path $ShortcutSm)  { Remove-Item -Force $ShortcutSm }
    if (Test-Path $ShortcutDt)  { Remove-Item -Force $ShortcutDt }
}

if ($Uninstall) { Remove-LinuxCAD; Log 'Done.'; exit 0 }

$Arch = if ([Environment]::Is64BitOperatingSystem) { 'x64' } else { Die 'unsupported CPU (need x64)' }
$Tmp = Join-Path $env:TEMP ("linuxcad-install-" + [Guid]::NewGuid())
New-Item -ItemType Directory -Force -Path $Tmp | Out-Null

try {
    $Zip = Join-Path $Tmp 'linuxcad.zip'
    if ($LocalZip) {
        if (-not (Test-Path $LocalZip)) { Die "local zip not found: $LocalZip" }
        Copy-Item $LocalZip $Zip
    } else {
        if (-not $ZipUrl) {
            $api = if ($Version) { "https://api.github.com/repos/$Repo/releases/tags/$Version" } else { "https://api.github.com/repos/$Repo/releases/latest" }
            Log "Querying GitHub: $api"
            $headers = @{ 'Accept' = 'application/vnd.github+json' }
            $release = Invoke-RestMethod -UseBasicParsing -Headers $headers -Uri $api
            $manifestAsset = $release.assets | Where-Object { $_.name -eq 'latest.json' } | Select-Object -First 1
            if ($manifestAsset) {
                try {
                    $manifest = Invoke-RestMethod -UseBasicParsing -Uri $manifestAsset.browser_download_url
                    $ZipUrl = $manifest."windows_$Arch"
                } catch { }
            }
            if (-not $ZipUrl) {
                $asset = $release.assets | Where-Object { $_.name -match "^LinuxCAD-for-Windows-.*-$Arch\.zip$" } | Select-Object -First 1
                if ($asset) { $ZipUrl = $asset.browser_download_url }
            }
            if (-not $ZipUrl) { Die "no LinuxCAD-for-Windows-*-$Arch.zip in release" }
        }
        Log "Downloading $ZipUrl"
        Invoke-WebRequest -UseBasicParsing -Uri $ZipUrl -OutFile $Zip
    }

    Remove-LinuxCAD
    New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

    Log "Expanding to $InstallDir..."
    Expand-Archive -Path $Zip -DestinationPath $InstallDir -Force

    $TargetBat = Join-Path $InstallDir 'bin\LinuxCAD.bat'
    if (-not (Test-Path $TargetBat)) {
        # Older zips may not include the wrapper; fall back to FreeCAD.exe.
        $TargetBat = Join-Path $InstallDir 'bin\FreeCAD.exe'
    }

    Log 'Creating shortcuts...'
    $WshShell = New-Object -ComObject WScript.Shell
    foreach ($lnk in @($ShortcutSm, $ShortcutDt)) {
        $sc = $WshShell.CreateShortcut($lnk)
        $sc.TargetPath = $TargetBat
        $sc.WorkingDirectory = (Join-Path $InstallDir 'bin')
        $sc.IconLocation = (Join-Path $InstallDir 'bin\FreeCAD.exe,0')
        $sc.Description = 'LinuxCAD'
        $sc.Save()
    }

    Log ''
    Log "LinuxCAD installed at $InstallDir"
    Log 'Open it from the Start menu, or run:'
    Log "  & '$TargetBat'"
} finally {
    if (Test-Path $Tmp) { Remove-Item -Recurse -Force $Tmp -ErrorAction SilentlyContinue }
}
