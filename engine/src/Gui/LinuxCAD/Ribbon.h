// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_RIBBON_H
#define GUI_LINUXCAD_RIBBON_H

#include <FCGlobal.h>
#include <QHash>
#include <QWidget>

class QTabBar;
class QStackedWidget;
class QSettings;
class QToolBar;
class QTimer;

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
class GuiExport Ribbon : public QWidget
{
    Q_OBJECT

public:
    explicit Ribbon(QWidget* parent = nullptr);
    ~Ribbon() override;

public Q_SLOTS:
    /// Rebuild from the active workbench's toolbars. Safe to call any time.
    void rebuild();

    /// Coalesce workbench swaps: FreeCAD reparents toolbar actions shortly after activation.
    void scheduleRebuild();

private Q_SLOTS:
    void onRebuildDebounce();
    void onTabChanged(int index);

private:
    void clearTabs();
    QWidget* buildPage(QToolBar* sourceToolBar);
    bool shouldSkipToolBar(QToolBar* tb) const;

    QTimer* rebuildDebounce_ = nullptr;
    QTabBar* tabBar_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QHash<int, QString> tabTitleByIndex_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_RIBBON_H
