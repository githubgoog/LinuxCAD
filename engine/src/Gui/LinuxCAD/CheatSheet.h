// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_CHEATSHEET_H
#define GUI_LINUXCAD_CHEATSHEET_H

#include <FCGlobal.h>
#include <QDialog>

class QLineEdit;
class QScrollArea;
class QString;
class QVBoxLayout;
class QWidget;

namespace Gui {
namespace LinuxCAD {

class CommandPalette;

/// Searchable, modal-ish keyboard cheat-sheet.
///
/// Lists every entry from `Shortcuts::curated()` grouped by category, with
/// a search box that fuzz-filters by command name + friendly label. Keys
/// are rendered in monospaced "key cap" pills.
///
/// Triggered via global `?` and `Ctrl+/` shortcuts installed by the shell.
class GuiExport CheatSheet : public QDialog
{
    Q_OBJECT

public:
    explicit CheatSheet(CommandPalette* palette = nullptr, QWidget* parent = nullptr);
    ~CheatSheet() override;

    /// Show centered over the parent window with the search box focused.
    void showOverlay();

protected:
    void keyPressEvent(QKeyEvent* ev) override;

private Q_SLOTS:
    void onFilterChanged(const QString& text);

private:
    void buildUi();
    void rebuildContent(const QString& filter);

    CommandPalette* palette_   = nullptr;
    QLineEdit*      search_    = nullptr;
    QScrollArea*    scroll_    = nullptr;
    QWidget*        content_   = nullptr;
    QVBoxLayout*    contentLayout_ = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_CHEATSHEET_H
