// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_CONTEXTBUILDER_H
#define GUI_LINUXCAD_AI_CONTEXTBUILDER_H

#include <FCGlobal.h>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace Gui {
namespace LinuxCAD {

/// Build a compact, privacy-conscious JSON snapshot of the user's current
/// modeling context. The snapshot is what gets sent to the AI provider so
/// it can suggest meaningful next operations.
///
/// Privacy: we never include geometry data files (STEP / IGES / mesh
/// vertices). We include object names + types + a light geometry summary
/// (e.g. "Pad: length=10mm", "Edge: straight, length=42mm") and the last
/// few command names from the undo stack. If `LinuxCAD/AI/RedactNames` is
/// true, names are replaced with stable hashes.
class GuiExport ContextBuilder
{
public:
    /// Build the snapshot from the current FreeCAD application state.
    static QJsonObject build();

    /// Build a snapshot intended for a per-document opt-out check.
    static bool perDocumentOptOut();
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_CONTEXTBUILDER_H
