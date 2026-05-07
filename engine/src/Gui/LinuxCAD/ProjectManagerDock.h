// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_PROJECTMANAGERDOCK_H
#define GUI_LINUXCAD_PROJECTMANAGERDOCK_H

#include <FCGlobal.h>
#include <QDockWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;

namespace Gui {
namespace LinuxCAD {

class ProjectManager;

/// Left-hand dock widget that lists the active LinuxCAD project's members
/// and surfaces project-level actions (add file, open in FreeCAD, ...).
class GuiExport ProjectManagerDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit ProjectManagerDock(ProjectManager* manager, QWidget* parent = nullptr);
    ~ProjectManagerDock() override;

private Q_SLOTS:
    void rebuild();
    void onFilterChanged(const QString& text);
    void onItemDoubleClicked(QTreeWidgetItem* item, int col);
    void onAddFile();
    void onSaveProject();
    void onCloseProject();

private:
    void buildUi();
    void applyFilter();
    void populateMembers();

    ProjectManager* manager_   = nullptr;

    QLabel*         header_    = nullptr;
    QLabel*         subHeader_ = nullptr;
    QLineEdit*      filter_    = nullptr;
    QTreeWidget*    tree_      = nullptr;
    QPushButton*    addBtn_    = nullptr;
    QPushButton*    saveBtn_   = nullptr;
    QPushButton*    closeBtn_  = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_PROJECTMANAGERDOCK_H
