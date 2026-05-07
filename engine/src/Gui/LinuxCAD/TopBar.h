// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_TOPBAR_H
#define GUI_LINUXCAD_TOPBAR_H

#include <FCGlobal.h>
#include <QToolBar>

class QAction;
class QLineEdit;
class QToolButton;
class QVBoxLayout;
class QWidget;

namespace Gui {
namespace LinuxCAD {

class Ribbon;
class WorkbenchDropdownButton;

/// LinuxCAD top bar.
///
/// Wave 2E rebuilds the top bar as a `QToolBar` whose only direct toolbar
/// widget is a `QVBoxLayout`-hosted shell with exactly two rows:
///
///  - Row 1: logo button -> workbench dropdown -> quick-search (stretches)
///           -> palette button -> spacer -> Undo / Redo -> AI badge -> user.
///  - Row 2: a container that hosts the `Ribbon*` widget installed by
///           `LinuxCadShell::install` via `setRibbonBody(...)`.
///
/// The previous separate Project dropdown, the QComboBox-based workbench
/// switcher, and the save indicator label have been removed: the project
/// menu now hangs off the logo button and the save state is reflected
/// elsewhere in the shell. `onActiveDocumentChanged()` is preserved as a
/// no-op slot so existing external connections still compile.
class GuiExport TopBar : public QToolBar
{
    Q_OBJECT

public:
    explicit TopBar(QWidget* parent = nullptr);
    ~TopBar() override;

    /// Embed a Ribbon widget into the second row of the TopBar shell.
    /// The ribbon's parent is reset to the row-2 container, so its layout
    /// position follows the TopBar through docking / undocking.
    void setRibbonBody(Ribbon* ribbon);
    Ribbon* ribbonBody() const;

    /// Ribbon row reacts to documents: inactive when none are open (plan).
    void setRibbonRowInteractive(bool enabled);

public Q_SLOTS:
    /// Retained as a no-op so existing connections (e.g. document changes)
    /// continue to compile after the save indicator was removed.
    void onActiveDocumentChanged();

    /// Update the AI status badge to reflect an integer state value
    /// matching `Gui::LinuxCAD::SuggestionEngine::State`.
    void onAiStateChanged(int state);

private Q_SLOTS:
    void onCommandPaletteRequested();
    void onAiBadgeClicked();
    void onUndo();
    void onRedo();
    void onQuickSearchEdited(const QString& text);

private:
    void buildLayout();
    void buildLogoMenu();
    QToolButton* makeIconButton(const char* iconName, const QString& tooltip, const char* slot);

    // --- Outer 2-row shell ----------------------------------------------
    QWidget*     outer_           = nullptr;
    QVBoxLayout* outerLayout_     = nullptr;
    QWidget*     row1_            = nullptr;
    QWidget*     row2_            = nullptr;

    // --- Row 1 widgets --------------------------------------------------
    QToolButton*              logoButton_         = nullptr;
    WorkbenchDropdownButton*  workbenchDropdown_  = nullptr;
    QLineEdit*                quickSearch_        = nullptr;
    QToolButton*              paletteButton_      = nullptr;
    QToolButton*              undoButton_         = nullptr;
    QToolButton*              redoButton_         = nullptr;
    QToolButton*              aiBadge_            = nullptr;
    QToolButton*              userButton_         = nullptr;

    // --- Row 2 hosted ribbon -------------------------------------------
    Ribbon*                   ribbonHosted_       = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_TOPBAR_H
