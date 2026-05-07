// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_WELCOMESCREEN_H
#define GUI_LINUXCAD_WELCOMESCREEN_H

#include <FCGlobal.h>
#include <QWidget>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLabel;

namespace Gui {
namespace LinuxCAD {

class ProjectManager;

/// Welcome / Start screen.
///
/// Shown as a top-level dialog the first time LinuxCAD launches without a
/// project, and reachable later via Project ▸ Welcome. It provides:
///  - Recent projects
///  - "New Project" with a small set of templates
///  - "Open Project"
///  - Direct entry point to FreeCAD's existing Start workbench (for users
///    who want the legacy experience without a managed project)
class GuiExport WelcomeScreen : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeScreen(ProjectManager* manager, QWidget* parent = nullptr);
    ~WelcomeScreen() override;

    /// Show as a centered, modal-ish overlay over the main window.
    void showCentered();

public Q_SLOTS:
    void refresh();

private Q_SLOTS:
    void onNewSketch();
    void onNewProject();
    void onOpenProject();
    void onRecentDoubleClicked(QListWidgetItem* item);
    void onOpenStartWorkbench();

private:
    void buildUi();

    ProjectManager* manager_     = nullptr;
    QLabel*         heroLabel_   = nullptr;
    QListWidget*    recentList_  = nullptr;
    QPushButton*    sketchBtn_   = nullptr;
    QPushButton*    newBtn_      = nullptr;
    QPushButton*    openBtn_     = nullptr;
    QPushButton*    startBtn_    = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_WELCOMESCREEN_H
