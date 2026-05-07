// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_SHORTCUTS_H
#define GUI_LINUXCAD_SHORTCUTS_H

#include <FCGlobal.h>
#include <QString>
#include <vector>

namespace Gui {
namespace LinuxCAD {

/// One curated keyboard shortcut.
struct ShortcutEntry
{
    const char* commandName;   ///< FreeCAD command id, e.g. "Sketcher_NewSketch"
    const char* keySequence;   ///< Key sequence, e.g. "S" or "Ctrl+/"
    const char* category;      ///< Display group, e.g. "Modeling"
    const char* friendlyName;  ///< User-facing label, falls back to the command's menu text
};

/// Apply LinuxCAD's curated, Fusion-aligned shortcut map.
///
/// The first call (per user profile) writes our defaults onto the matching
/// FreeCAD `Command`s via `setShortcut`. Subsequent calls are no-ops so we
/// never clobber user remappings.
class GuiExport Shortcuts
{
public:
    static void applyDefaults();
    static void applyForce();

    /// Returns a stable, ordered list of the curated shortcuts. Used by the
    /// CheatSheet overlay to render itself.
    static const std::vector<ShortcutEntry>& curated();

    /// Repeat the previously executed command. Wired to `Space`.
    static void repeatLast();

    /// Record a command name as the most recently executed - called by
    /// CommandPalette / CheatSheet when they invoke a command.
    static void recordExecuted(const QString& commandName);
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_SHORTCUTS_H
