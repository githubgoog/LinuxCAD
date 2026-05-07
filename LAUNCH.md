# Launching LinuxCAD on Linux

## Two paths in `~/Coding` — not two copies

You may see both:

- `~/Coding/Linux CAD` — the real git checkout (folder name has a **space**).
- `~/Coding/LinuxCAD` — a **symlink** to the same folder (no space).

They are the **same repository** on disk. The symlink exists so builds and tools that mishandle spaces in paths still work. Prefer opening the project from:

```text
/home/sabastian/Coding/LinuxCAD
```

in your IDE and terminal when working on LinuxCAD.

## Fastest way to open the app (already built)

The `linuxcad` launcher **prefers a binary built from this repo** (`build/_out` / `build/_install`). That is the FreeCAD-fork LinuxCAD.

An old **Electron-era** build may still live at `~/.local/bin/LinuxCAD.AppImage`. That is **not** the same app. The launcher ignores it unless you opt in:

```bash
export LINUXCAD_USE_SYSTEM_APPIMAGE=1
linuxcad
```

Or point to any AppImage explicitly:

```bash
export LINUXCAD_APPIMAGE="$HOME/Downloads/LinuxCAD-1.0.0-x86_64.AppImage"
linuxcad
```

AppImages produced by this repo’s packaging script are picked up automatically from `build/_packages/` once you have built them.

## Building from source on Ubuntu/Debian

The README lists core packages. If the link step fails with **cannot find -lTKSTEP** (or similar `TK*` libraries), install Open CASCADE **data exchange** dev files:

```bash
sudo apt install libocct-data-exchange-dev
```

Then rebuild from the **no-space** path:

```bash
cd ~/Coding/LinuxCAD
./build/build-linux.sh --install
```

## Menu / desktop shortcut

The `.desktop` file should use:

- `Exec=/home/sabastian/.local/bin/linuxcad` (or your repo `./linuxcad` copied there)
- `Path=/home/sabastian/Coding/LinuxCAD` (symlink path avoids CMake/Python issues with spaces)

After changing dependencies or the launcher, run:

```bash
update-desktop-database ~/.local/share/applications
```

## Python / Shiboken (optional warnings)

If CMake warns that the `shiboken6` Python module is missing, install PySide/Shiboken for Python 3 (package names vary by distro) or use pip in a user environment:

```bash
sudo apt install python3-pip
python3 -m pip install --user shiboken6 PySide6-Essentials
```
