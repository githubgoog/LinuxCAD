// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_THEMEPICKER_H
#define GUI_LINUXCAD_THEMEPICKER_H

#include <FCGlobal.h>
#include <QDialog>
#include <QString>

class QListWidget;

namespace Gui {
namespace LinuxCAD {

/// User-facing dialog for choosing among bundled and user-installed themes.
///
/// Populates from manifest discovery: bundled qrc themes and user
/// overrides in \c ~/.config/LinuxCAD/themes/.
class GuiExport ThemePicker : public QDialog
{
    Q_OBJECT

public:
    explicit ThemePicker(QWidget* parent = nullptr);
    ~ThemePicker() override;

    /// Resolve and populate available themes from Theme discovery.
    void refresh();

Q_SIGNALS:
    void themeChosen(const QString& id);

private:
    void buildUi();

    QListWidget* list_ = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_THEMEPICKER_H
