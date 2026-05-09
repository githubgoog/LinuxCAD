// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_FIRST_RUN_WIZARD_H
#define GUI_LINUXCAD_FIRST_RUN_WIZARD_H

#include <FCGlobal.h>

class QWidget;

namespace Gui {
namespace LinuxCAD {

/// One-shot first-launch onboarding (theme, units, navigation, AI consent).
///
/// Completion is gated by {@code LinuxCAD/FirstRunWizardCompletedV1}.
class GuiExport FirstRunWizard
{
public:
    static constexpr const char* kCompletedKey = "LinuxCAD/FirstRunCompleted";

    /// If not yet completed, runs the wizard modally above {@p parent}.
    static void promptIfNeeded(QWidget* parent);

    /// Clears completion and opens the wizard (logo menu \"Re-run setup\").
    static void runAgain(QWidget* parent);

    /// Apply sensible defaults on first launch without showing UI:
    ///   theme=dark, units=metric, navigation=LinuxCAD, AI=mock+disabled.
    /// Sets `LinuxCAD/FirstRunCompleted` so promptIfNeeded() and itself
    /// become no-ops on subsequent launches.
    static void applySilentDefaults();

private:
    FirstRunWizard() = delete;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_FIRST_RUN_WIZARD_H
