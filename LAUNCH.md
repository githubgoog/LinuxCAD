# LinuxCAD developer launch notes

> **Most users should not need this file.** The recommended install path is
> `bash install.sh` (see [README.md](README.md)). This document is for
> contributors building LinuxCAD from source.

## Launcher resolution order

`./linuxcad` is now a **user launcher**:

1. `$LINUXCAD_APPIMAGE` if set and present.
2. Newest `LinuxCAD-*.AppImage` under `~/.local/bin/` (installed by `install.sh`).

It does **not** attempt to compile from source. By default it enables safe
software rendering (`LINUXCAD_SAFE_MODE=1`) to reduce GPU-related hangs.

Use `./linuxcad --self-check` for a quick diagnostics summary.

For contributors, use `./linuxcad-dev` (repo build + optional auto-build).

If you want to point the launcher at a specific AppImage:

```bash
export LINUXCAD_APPIMAGE="$HOME/Downloads/LinuxCAD-1.0.0-x86_64.AppImage"
linuxcad
```

## Repository path

Spaces in the path are fine — LinuxCAD's CMake patches (see
[patches/README.md](patches/README.md)) quote paths through Python codegen
and file-copy steps.

If your path still trips up some external tool, the build scripts
automatically place `build/_out` and `build/_install` under
`$XDG_CACHE_HOME/linuxcad/build-<hash>/` whenever the repository path
contains whitespace. Override with `LINUXCAD_BUILD_DIR` and
`LINUXCAD_INSTALL_DIR` if you want a fixed location.

## Building on Ubuntu/Debian

```bash
bash scripts/install-linux-deps.sh   # build dependencies
./build/build-linux.sh --install
```

If the link step fails with `cannot find -lTKSTEP` (or similar `TK*`
libraries), install OpenCASCADE **data exchange** dev files:

```bash
sudo apt install libocct-data-exchange-dev
```

## Menu / desktop shortcut for a dev build

If you want a `.desktop` entry that launches your **local checkout's** build
(rather than the user-installed AppImage), point `Exec` at the repo's
`linuxcad-dev` script:

```ini
[Desktop Entry]
Type=Application
Name=LinuxCAD (dev)
Exec=/path/to/your/LinuxCAD/linuxcad-dev %F
Icon=linuxcad
Categories=Graphics;Engineering;3DGraphics;
```

Drop into `~/.local/share/applications/` and run:

```bash
update-desktop-database ~/.local/share/applications
```

The user-facing entry created by `install.sh` is independent of this and
points at the AppImage.

## Python / Shiboken (optional warnings)

If CMake warns that `shiboken6` is missing, install PySide/Shiboken for
Python 3 (package names vary by distro) or use pip:

```bash
sudo apt install python3-pip
python3 -m pip install --user shiboken6 PySide6-Essentials
```
