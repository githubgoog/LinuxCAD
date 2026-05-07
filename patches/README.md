# LinuxCAD patches against FreeCAD 1.1.1

LinuxCAD is a downstream fork of FreeCAD (LGPL-2.1+). This directory tracks
the deliberate, minimal modifications LinuxCAD applies to the upstream
FreeCAD source under `FreeCAD-main/`. Every patch listed here is reproduced
inline so that source recipients can audit them quickly.

## Patches

### 0001-add-linuxcad-shell-module.patch
Adds the new `src/Gui/LinuxCAD/` source directory and resource bundle:
top bar, project manager dock, welcome screen, theme, command palette,
project model, and the `Gui::LinuxCAD::install()` entry point.

### 0002-wire-linuxcad-into-gui-cmake.patch
Adds `add_subdirectory(LinuxCAD)` to `src/Gui/CMakeLists.txt`, and appends
the LinuxCAD sources/resources to the `FreeCADGui` shared library target.

### 0003-install-shell-from-mainwindow.patch
Adds `#include "LinuxCAD/LinuxCadShell.h"` in `src/Gui/MainWindow.cpp` and
calls `Gui::LinuxCAD::install(this)` at the end of the `MainWindow`
constructor. No other behavior in MainWindow is altered.

### 0004-rebrand-main-gui.patch
Changes `App::Application::Config()["ExeName"]` to `LinuxCAD`, the
maintainer URL to the LinuxCAD repository, and the `DesktopFileName` to
`org.linuxcad.LinuxCAD`. `ExeVendor` is intentionally kept as `FreeCAD` so
the user's existing FreeCAD settings, addons, and recent files continue to
work transparently.

## How to regenerate

To produce a clean diff against vanilla FreeCAD 1.1.1:

```bash
git -C FreeCAD-main fetch --depth=1 origin tag 1.1.1
git -C FreeCAD-main diff 1.1.1..HEAD -- src/Gui src/Main > patches/0000-current.patch
```

## License notice

Our additions under `FreeCAD-main/src/Gui/LinuxCAD/` are licensed
LGPL-2.1-or-later, matching the surrounding `FreeCADGui` library. They link
into the same shared object and are distributable under the same terms as
the rest of the project.
