# Third-party notices

LinuxCAD is built on top of FreeCAD (used as the modeling engine) and the
libraries FreeCAD depends on. Each component below is distributed under its
own license; LinuxCAD's distribution complies with each.

## FreeCAD (engine)

- License: LGPL-2.1-or-later
- Source: https://github.com/FreeCAD/FreeCAD (vendored at tag `1.1.1`)
- LinuxCAD ships the full engine source under `engine/`. Modifications
  applied by LinuxCAD are listed in `patches/README.md`.
- LinuxCAD links against the engine as a set of dynamic libraries
  (`libFreeCADBase`, `libFreeCADApp`, `libFreeCADGui`, etc.). The LGPL
  permits this and also permits relinking against modified versions.

## OpenCASCADE Technology

- License: LGPL-2.1 with the OCCT exception
- Source: https://dev.opencascade.org/
- Linked dynamically.

## Coin3D

- License: BSD-3-Clause
- Source: https://github.com/coin3d/coin
- Linked dynamically.

## Qt

- License: LGPL-3.0
- Linked dynamically. LinuxCAD does not statically link any Qt module.

## Other

See the `LICENSE` file at the project root and `engine/LICENSE` for the
full engine license, plus `engine/src/3rdParty/*` for individual third-party
submodule licenses (GSL, OndselSolver, googletest).

LinuxCAD's own additions under `engine/src/Gui/LinuxCAD/`, `branding/`,
`build/`, `packaging/`, and `patches/` are licensed LGPL-2.1-or-later,
matching the FreeCADGui library they extend.
