# LinuxCAD patches against the FreeCAD engine (1.1.1)

LinuxCAD is a downstream fork of FreeCAD (LGPL-2.1+). This directory tracks
the deliberate, minimal modifications LinuxCAD applies to the vendored
engine source under `engine/`. Every patch listed here is reproduced inline
so that source recipients can audit them quickly.

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

### 0005-quote-paths-in-cmake-codegen.patch
Quotes `Python3_EXECUTABLE`, `CMAKE_COMMAND`, and script paths in
`cMake/FreeCadMacros.cmake` (`fc_copy_file_if_different`, `generate_from_xml`,
`generate_from_py_impl`) and in `cMake/FreeCAD_Helpers/CreatePackagingTargets.cmake`
so configure/build steps work when `CMAKE_SOURCE_DIR` or related paths contain
spaces. Adds `fc_to_exec_path()` so `file(TO_NATIVE_PATH)` is used only on
Windows; on Unix it was turning spaces into backslash escapes and breaking
`execute_process` / Python (`generate.py` not found under `Linux CAD`).
Paired with `build/common.sh`, which can redirect build/install trees to
`$XDG_CACHE_HOME/linuxcad/...` when the repo path contains whitespace.

## How to regenerate

To produce a clean diff against vanilla FreeCAD 1.1.1:

```bash
git -C engine fetch --depth=1 origin tag 1.1.1
git -C engine diff 1.1.1..HEAD -- src/Gui src/Main > patches/0000-current.patch
```

## License notice

Our additions under `engine/src/Gui/LinuxCAD/` are licensed
LGPL-2.1-or-later, matching the surrounding `FreeCADGui` library. They link
into the same shared object and are distributable under the same terms as
the rest of the project.
