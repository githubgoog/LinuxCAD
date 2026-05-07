// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_TOPBAR_H
#define GUI_LINUXCAD_TOPBAR_H

#include <FCGlobal.h>
#include <QToolBar>

class QAction;
class QComboBox;
class QLabel;
class QToolButton;
class QLineEdit;

namespace Gui {
namespace LinuxCAD {

/// LinuxCAD top bar.
///
/// Replaces the visual role of FreeCAD's classic QMenuBar + standard toolbar.
/// Contains: project menu, workbench switcher, command palette opener,
/// undo/redo, save indicator, and a user/profile area.
///
/// Implementation note: we are a QToolBar so we get docking + chrome handling
/// for free, but our visual identity is configured via QSS to look like a
/// modern app top bar rather than a Qt toolbar.
class GuiExport TopBar : public QToolBar
{
    Q_OBJECT

public:
    explicit TopBar(QWidget* parent = nullptr);
    ~TopBar() override;

public Q_SLOTS:
    /// Refresh the workbench switcher when workbenches change.
    void refreshWorkbenchList();
    /// Highlight the active workbench when it changes.
    void onWorkbenchActivated(const QString& name);
    /// Update save indicator state.
    void onActiveDocumentChanged();

private Q_SLOTS:
    void onProjectButtonClicked();
    void onWorkbenchSelectionChanged(int index);
    void onCommandPaletteRequested();
    void onUndo();
    void onRedo();
    void onQuickSearchEdited(const QString& text);

private:
    void buildLayout();
    void buildProjectMenu();
    QToolButton* makeIconButton(const char* iconName, const QString& tooltip, const char* slot);

    QToolButton* projectButton_      = nullptr;
    QComboBox*   workbenchSwitcher_  = nullptr;
    QToolButton* paletteButton_      = nullptr;
    QLineEdit*   quickSearch_        = nullptr;
    QToolButton* undoButton_         = nullptr;
    QToolButton* redoButton_         = nullptr;
    QLabel*      saveIndicator_      = nullptr;
    QToolButton* userButton_         = nullptr;

    bool         updatingSwitcher_   = false;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_TOPBAR_H
