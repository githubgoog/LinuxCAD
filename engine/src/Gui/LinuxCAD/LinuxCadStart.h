// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef GUI_LINUXCAD_START_H
#define GUI_LINUXCAD_START_H

#include <QStringList>
#include <QWidget>

class QLabel;
class QVBoxLayout;

namespace Gui {
class MainWindow;
}

namespace Gui {
namespace LinuxCAD {

class LinuxCadStart : public QWidget
{
    Q_OBJECT

public:
    explicit LinuxCadStart(Gui::MainWindow* mainWindow, QWidget* parent = nullptr);

    void setHostWidget(QWidget* hostWidget);
    void syncToHost();
    void refreshRecents();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildUi();
    void clearRecentEntries();
    void addRecentEntry(const QString& filePath);
    void openRecentPath(const QString& filePath);

    static QStringList loadRecentPaths(int maxEntries);

private:
    Gui::MainWindow* mainWindow_ = nullptr;
    QWidget*         hostWidget_ = nullptr;
    QLabel*          logoLabel_ = nullptr;
    QVBoxLayout*     recentListLayout_ = nullptr;
    QWidget*         recentEmptyState_ = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_START_H
