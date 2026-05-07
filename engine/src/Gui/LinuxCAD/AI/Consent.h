// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_CONSENT_H
#define GUI_LINUXCAD_AI_CONSENT_H

#include <FCGlobal.h>

class QWidget;

namespace Gui {
namespace LinuxCAD {

/// First-run consent flow. Returns once the user has either consented or
/// declined; the result is persisted in QSettings.
class GuiExport Consent
{
public:
    /// Show the consent dialog if the user hasn't already given (or refused)
    /// consent. No-op on subsequent calls.
    static void promptIfNeeded(QWidget* parent);

    /// Returns true if the user has explicitly granted consent.
    static bool isGranted();

    /// Returns true if the user has been prompted (granted or refused).
    static bool wasPrompted();

    /// Per-doc opt-out helpers.
    static bool isDocOptedOut(const char* docName);
    static void setDocOptedOut(const char* docName, bool optedOut);
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_CONSENT_H
