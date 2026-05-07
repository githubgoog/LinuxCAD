# LinuxCAD icons & splash

Drop the following assets into this folder. The build system picks them up
automatically (see `branding/apply-branding.sh` and the CMake post-step).

Required:

- `linuxcad.svg`              — primary app icon (scalable, square)
- `linuxcad-16.png`           — 16x16
- `linuxcad-32.png`           — 32x32
- `linuxcad-48.png`           — 48x48
- `linuxcad-64.png`           — 64x64
- `linuxcad-128.png`          — 128x128
- `linuxcad-256.png`          — 256x256
- `linuxcad-doc.svg`          — file-association icon for `.lcadproj`
- `linuxcad-splash.png`       — splash image (about 600x300, FreeCAD default placement)
- `linuxcad-about.png`        — about-dialog image (about 600x300)

Optional / dev variants:

- `linuxcad-aboutdev.png`     — about image used for development builds
- `linuxcad-128.icns`         — macOS icon bundle (generated from PNGs)
- `linuxcad.ico`              — Windows icon

When these are present, `branding/apply-branding.sh` will copy them on top of
the corresponding FreeCAD assets under `FreeCAD-main/src/Gui/Icons/` and
register them as `freecad`/`freecadsplash`/`freecadabout` so the existing
runtime config in `MainGui.cpp` resolves to LinuxCAD's artwork without any
code changes.

If a file is missing, the corresponding FreeCAD asset is left in place so
the build still succeeds.
