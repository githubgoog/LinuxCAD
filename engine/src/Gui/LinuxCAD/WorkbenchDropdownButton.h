// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_WORKBENCH_DROPDOWN_BUTTON_H
#define GUI_LINUXCAD_WORKBENCH_DROPDOWN_BUTTON_H

#include <FCGlobal.h>
#include <QToolButton>

class QEvent;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QObject;
class QWidget;

namespace Gui {
namespace LinuxCAD {

/// Workbench picker rendered as a flat QToolButton in the LinuxCAD TopBar.
///
/// Replaces the QComboBox-based switcher used in earlier waves. Clicking the
/// button opens a Qt::Popup with a filter QLineEdit on top and a QListWidget
/// of all known workbenches below. Selecting a row calls
/// `Gui::Application::Instance->activateWorkbench(...)` and closes the popup.
///
/// The button itself shows the current workbench's icon, its display label
/// ("Part Design", "Sketcher", ...) and a unicode chevron.
class GuiExport WorkbenchDropdownButton : public QToolButton
{
    Q_OBJECT

public:
    explicit WorkbenchDropdownButton(QWidget* parent = nullptr);
    ~WorkbenchDropdownButton() override;

    /// Sync the button face to whatever workbench FreeCAD currently considers
    /// active. Safe to call before the popup is ever opened.
    void syncToActive();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
    void onClicked();
    void onFilterTextChanged(const QString& text);
    void onItemActivated(QListWidgetItem* item);

private:
    void buildFace();
    void ensurePopup();
    void rebuildModel();
    void applyFilter(const QString& text);
    void positionPopup();
    void activateRow(QListWidgetItem* item);
    void workbenchLabel(const QString& wb);

    QLabel*      iconLabel_      = nullptr;
    QLabel*      textLabel_      = nullptr;
    QLabel*      chevronLabel_   = nullptr;
    QHBoxLayout* faceLayout_     = nullptr;

    QWidget*     popup_          = nullptr;
    QLineEdit*   filter_         = nullptr;
    QListWidget* list_           = nullptr;

    bool         updatingDropdown_ = false;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_WORKBENCH_DROPDOWN_BUTTON_H
