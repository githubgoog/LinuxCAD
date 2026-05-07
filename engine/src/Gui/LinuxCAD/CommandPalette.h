// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_COMMANDPALETTE_H
#define GUI_LINUXCAD_COMMANDPALETTE_H

#include <FCGlobal.h>
#include <QDialog>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QKeyEvent;

namespace Gui {
class Command;
}

namespace Gui {
namespace LinuxCAD {

/// Cmd/Ctrl-K command palette.
///
/// Lists every registered FreeCAD command, fuzzy-filters them by name,
/// menu text and tooltip, and runs the chosen one via Command::invoke.
class GuiExport CommandPalette : public QDialog
{
    Q_OBJECT

public:
    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override;

public Q_SLOTS:
    void showPalette(const QString& initialQuery = QString());

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private Q_SLOTS:
    void onQueryChanged(const QString& q);
    void onItemActivated(QListWidgetItem* item);

private:
    void buildUi();
    void rebuildIndex();
    // NB: not "runCommand" - that name is a function-like macro in
    // Gui/Command.h which would mangle this declaration once that header
    // is included transitively (e.g. via LinuxCadShell.cpp).
    void executeSelected(Gui::Command* cmd);

    QLineEdit*   query_     = nullptr;
    QListWidget* list_      = nullptr;
    QLabel*      footer_    = nullptr;

    struct Entry {
        Gui::Command* cmd;
        QString       name;
        QString       menuText;
        QString       tooltip;
        QString       haystack; // cached lower-cased combined text for fuzzy matching
    };
    QList<Entry> all_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_COMMANDPALETTE_H
