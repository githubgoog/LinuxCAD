// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_RIBBON_H
#define GUI_LINUXCAD_RIBBON_H

#include <FCGlobal.h>
#include <QToolBar>

class QHBoxLayout;
class QScrollArea;
class QWidget;

namespace Gui {
namespace LinuxCAD {

/// Workbench-aware ribbon.
///
/// Sits on a second toolbar row directly under the LinuxCAD TopBar. On every
/// workbench activation it walks the MainWindow's QToolBars (which FreeCAD
/// has just populated for the active workbench), hides them, and re-renders
/// each as a labeled Fusion-style group of QToolButtons.
///
/// The ribbon never owns the QActions - it just shows them - so all clicks
/// flow through FreeCAD's existing CommandManager unchanged.
class GuiExport Ribbon : public QToolBar
{
    Q_OBJECT

public:
    explicit Ribbon(QWidget* parent = nullptr);
    ~Ribbon() override;

public Q_SLOTS:
    /// Rebuild from the active workbench's toolbars. Safe to call any time.
    void rebuild();

private Q_SLOTS:
    void scheduleRebuild();

private:
    void clearGroups();
    QWidget* buildGroup(QToolBar* sourceToolBar);
    bool shouldSkipToolBar(QToolBar* tb) const;

    QScrollArea* scroll_   = nullptr;
    QWidget*     content_  = nullptr;
    QHBoxLayout* row_      = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_RIBBON_H
