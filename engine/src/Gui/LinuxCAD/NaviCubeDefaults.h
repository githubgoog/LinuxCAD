// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_NAVICUBEDEFAULTS_H
#define GUI_LINUXCAD_NAVICUBEDEFAULTS_H

#include <FCGlobal.h>

namespace Gui {
namespace LinuxCAD {

/// Apply LinuxCAD's opinionated NaviCube defaults exactly once per user
/// profile. Subsequent invocations are no-ops so we never clobber a user
/// who has tuned the cube themselves.
///
/// The defaults are written into FreeCAD's `BaseApp/Preferences/NaviCube`
/// parameter group, the same place the existing NaviCube reads them from,
/// so polish carries through every viewer instance with no extra wiring.
class GuiExport NaviCubeDefaults
{
public:
    /// One-shot apply. Idempotent - guarded by a settings flag.
    static void applyOnce();

    /// Force-apply (used by the "Reset NaviCube" preference button later).
    static void applyForce();
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_NAVICUBEDEFAULTS_H
