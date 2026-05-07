// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_VIEWWIDGETSOVERLAY_H
#define GUI_LINUXCAD_VIEWWIDGETSOVERLAY_H

#include <FCGlobal.h>
#include <QObject>
#include <QPointer>
#include <QWidget>

class QHBoxLayout;
class QToolButton;

namespace Gui {
class MainWindow;
}

namespace Gui {
namespace LinuxCAD {

/// Floating cluster of view-control buttons (Home / Fit-all / Isometric /
/// Ortho-Perspective toggle) that sits in the top-right corner of every 3D
/// viewport, just above FreeCAD's NaviCube.
///
/// Each button invokes an existing FreeCAD Std_View* command, so we don't
/// duplicate any logic; we just give the user a fast, themed shortcut.
///
/// One overlay instance is parented to MainWindow; we discover and decorate
/// every View3DInventor widget as it shows up via an event filter on the
/// MainWindow.
class ViewClusterWidget;

class GuiExport ViewWidgetsOverlay : public QObject
{
    Q_OBJECT

public:
    explicit ViewWidgetsOverlay(Gui::MainWindow* mw);
    ~ViewWidgetsOverlay() override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void scanAndAttach();
    void attachToViewer(QWidget* viewer);

    QPointer<Gui::MainWindow>             mainWindow_;
    QList<QPointer<ViewClusterWidget>>    clusters_;
};

/// The actual floating button cluster. Public only so MOC can pick it up;
/// callers should go through ViewWidgetsOverlay.
class ViewClusterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ViewClusterWidget(QWidget* parent);
    ~ViewClusterWidget() override;

    void reposition();

protected:
    void resizeEvent(QResizeEvent* ev) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private Q_SLOTS:
    void onHome();
    void onFitAll();
    void onIsometric();
    void onToggleOrtho();

private:
    void buildUi();
    QToolButton* makeButton(const QString& label,
                            const QString& tooltip,
                            const char* iconHint,
                            const char* slot);

    QHBoxLayout* row_       = nullptr;
    QToolButton* homeBtn_   = nullptr;
    QToolButton* fitBtn_    = nullptr;
    QToolButton* isoBtn_    = nullptr;
    QToolButton* orthoBtn_  = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_VIEWWIDGETSOVERLAY_H
