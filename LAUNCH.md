# LinuxCAD developer launch notes

> **Most users should not need this file.** The recommended path is one of
> the per-OS installers in [README.md](README.md):
> `install.sh` (Linux) / `install.command` (macOS) / `install.ps1` (Windows).
> This document is for contributors building LinuxCAD from source via pixi.

## The two launchers

| Script           | Purpose                                              |
| ---------------- | ---------------------------------------------------- |
| `./linuxcad`     | User launcher; runs the OS-installed product only.   |
| `./linuxcad-dev` | Developer launcher; pixi-based build + run.          |

`./linuxcad` resolves the installed product per OS:

- **Linux** — newest `~/.local/bin/LinuxCAD-for-Linux-*.AppImage` (or
  `$LINUXCAD_APPIMAGE`).
- **macOS** — `/Applications/LinuxCAD.app` (or `$LINUXCAD_APP`).
- **Windows** — handled by `install.ps1` shortcuts; the bash launcher
  is not used.

It defaults to safe software rendering on Linux
(`LINUXCAD_SAFE_MODE=1`) to reduce 49%-startup-hang reports. Disable
with `LINUXCAD_SAFE_MODE=0`.

`./linuxcad --self-check` prints a quick diagnostics summary on Linux
and macOS.

## Building from source (pixi)

A single recipe ([pixi.toml](pixi.toml)) produces the same build on
Linux, macOS, and Windows. No `apt` / `brew` / `LibPack` involved —
everything comes from `conda-forge`.

```bash
# One-time install of pixi.
curl -fsSL https://pixi.sh/install.sh | bash

# Build, install, and launch.
./linuxcad-dev

# Or, equivalently:
pixi run linuxcad-release
pixi run launch
```

To produce the OS-native artifact (`.AppImage` / `.dmg` / `.zip`) into
`dist/`:

```bash
pixi run package          # picks the right per-OS task automatically
# or
./linuxcad-dev --package
```

## Common pixi tasks

| Task                  | What it does                                    |
| --------------------- | ----------------------------------------------- |
| `apply-branding`      | Overlay `branding/icons/` onto the engine.      |
| `configure-release`   | CMake configure using engine's conda preset.    |
| `build-release`       | Compile.                                        |
| `install-release`     | `cmake --install` into `.pixi/envs/default/`.   |
| `linuxcad-release`    | The four steps above, in order.                 |
| `package`             | Build the OS artifact into `dist/`.             |
| `launch`              | Run the just-installed LinuxCAD.                |

The pixi env (`.pixi/envs/default/`) takes ~3-4 GB on first install
but is cached locally and in CI on subsequent runs.

## Menu / desktop shortcut for a dev build

If you want a `.desktop` entry that launches your **local checkout's**
build rather than the OS-installed product, point `Exec` at
`linuxcad-dev`:

```ini
[Desktop Entry]
Type=Application
Name=LinuxCAD (dev)
Exec=/path/to/your/LinuxCAD/linuxcad-dev %F
Icon=linuxcad
Categories=Graphics;Engineering;3DGraphics;
```

Drop into `~/.local/share/applications/` and refresh:

```bash
update-desktop-database ~/.local/share/applications
```

The user-facing entry created by `install.sh` is independent of this
and points at the installed AppImage.

## Repository path

Spaces in the path are fine — every CMake/Python codegen path used by
the engine is quoted (see [patches/README.md](patches/README.md)). The
pixi build also writes intermediate artifacts under
`engine/build/release/` regardless of where the repo lives.
