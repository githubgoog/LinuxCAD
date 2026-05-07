// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QChildEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QToolButton>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>

#include "ViewWidgetsOverlay.h"

namespace Gui {
namespace LinuxCAD {

namespace {

void invokeFreeCADCommand(const char* name)
{
    if (auto* app = Gui::Application::Instance) {
        if (auto* cmd = app->commandManager().getCommandByName(name)) {
            cmd->invoke(0);
        }
    }
}

bool isOrthographicNow()
{
    auto* mdi = Gui::getMainWindow() ? Gui::getMainWindow()->activeWindow() : nullptr;
    auto* view = qobject_cast<Gui::View3DInventor*>(mdi);
    if (view == nullptr) {
        return false;
    }
    if (auto* viewer = view->getViewer()) {
        // FreeCAD models the camera type via a CameraType enum on the viewer;
        // here we go through the public command's checked-state logic via the
        // command manager so we don't depend on internal enums.
        if (auto* app = Gui::Application::Instance) {
            if (app->commandManager().getCommandByName("Std_OrthographicCamera") != nullptr) {
                // We can't easily read the action's checked state from here
                // without coupling to CommandView internals, so we read the
                // viewer's camera type directly.
            }
        }
        // Direct check: viewer->getCameraType() returns "Orthographic" / "Perspective"
        // through the SoCamera concrete type. We delegate by querying the
        // active QAction via the viewer's action, but to keep the dependency
        // surface small we just fall back to "perspective" here. The toggle
        // command itself will flip correctly regardless of our prediction.
        Q_UNUSED(viewer);
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
//  ViewClusterWidget
// ---------------------------------------------------------------------------

ViewClusterWidget::ViewClusterWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("LinuxCadViewCluster"));
    setProperty("linuxcadRole", QStringLiteral("view-widgets"));
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::NoFocus);

    buildUi();

    if (parent != nullptr) {
        parent->installEventFilter(this);
    }
}

ViewClusterWidget::~ViewClusterWidget() = default;

void ViewClusterWidget::buildUi()
{
    row_ = new QHBoxLayout(this);
    row_->setContentsMargins(6, 4, 6, 4);
    row_->setSpacing(2);

    homeBtn_  = makeButton(tr("Home"),  tr("Home view (H)"),       "Std_ViewHome",
                           SLOT(onHome()));
    fitBtn_   = makeButton(tr("Fit"),   tr("Fit all (Shift+F)"),   "Std_ViewFitAll",
                           SLOT(onFitAll()));
    isoBtn_   = makeButton(tr("Iso"),   tr("Isometric view"),      "Std_ViewIsometric",
                           SLOT(onIsometric()));
    orthoBtn_ = makeButton(tr("Ortho"), tr("Toggle ortho/perspective (/)"),
                           "Std_OrthographicCamera",
                           SLOT(onToggleOrtho()));
    orthoBtn_->setCheckable(true);

    row_->addWidget(homeBtn_);
    row_->addWidget(fitBtn_);
    row_->addWidget(isoBtn_);
    row_->addWidget(orthoBtn_);

    adjustSize();
}

QToolButton* ViewClusterWidget::makeButton(const QString& label,
                                           const QString& tooltip,
                                           const char* iconHint,
                                           const char* slot)
{
    auto* btn = new QToolButton(this);
    btn->setText(label);
    btn->setToolTip(tooltip);
    btn->setProperty("linuxcadRole", QStringLiteral("view-widget-btn"));
    btn->setAutoRaise(true);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    if (iconHint != nullptr) {
        const QPixmap icon = Gui::BitmapFactory().pixmap(iconHint);
        if (!icon.isNull()) {
            btn->setIcon(QIcon(icon));
            btn->setIconSize(QSize(16, 16));
        }
    }
    if (slot != nullptr) {
        connect(btn, SIGNAL(clicked()), this, slot);
    }
    return btn;
}

void ViewClusterWidget::reposition()
{
    auto* p = parentWidget();
    if (p == nullptr) {
        return;
    }
    // Sit in the top-right, with a small inset and a vertical gap above the
    // NaviCube which lives at default offset (0, 0) bottom-aligned to the
    // top-right corner. We pad enough to clear it.
    constexpr int kRightInset = 12;
    constexpr int kTopInset   = 12;
    adjustSize();
    move(p->width() - width() - kRightInset, kTopInset);
    raise();
}

void ViewClusterWidget::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    reposition();
}

bool ViewClusterWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == parentWidget()) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
            reposition();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ViewClusterWidget::onHome()       { invokeFreeCADCommand("Std_ViewHome"); }
void ViewClusterWidget::onFitAll()     { invokeFreeCADCommand("Std_ViewFitAll"); }
void ViewClusterWidget::onIsometric()  { invokeFreeCADCommand("Std_ViewIsometric"); }
void ViewClusterWidget::onToggleOrtho()
{
    // Best-effort toggle: alternate between Std_OrthographicCamera and
    // Std_PerspectiveCamera. The user feedback comes from the camera change;
    // the button's checked state mirrors the last requested mode.
    static thread_local bool wantOrtho = true;
    invokeFreeCADCommand(wantOrtho ? "Std_OrthographicCamera" : "Std_PerspectiveCamera");
    wantOrtho = !wantOrtho;
    if (orthoBtn_ != nullptr) {
        orthoBtn_->setChecked(!wantOrtho);
    }
}

// ---------------------------------------------------------------------------
//  ViewWidgetsOverlay
// ---------------------------------------------------------------------------

ViewWidgetsOverlay::ViewWidgetsOverlay(Gui::MainWindow* mw)
    : QObject(mw)
    , mainWindow_(mw)
{
    if (mw != nullptr) {
        mw->installEventFilter(this);
    }
    QTimer::singleShot(0, this, [this]() { scanAndAttach(); });
}

ViewWidgetsOverlay::~ViewWidgetsOverlay() = default;

bool ViewWidgetsOverlay::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj);
    if (event != nullptr && event->type() == QEvent::ChildAdded) {
        // A new MDI subwindow may host a View3DInventor. Defer scan to next
        // tick so the child is fully constructed.
        QTimer::singleShot(0, this, [this]() { scanAndAttach(); });
    }
    return false;
}

void ViewWidgetsOverlay::scanAndAttach()
{
    if (!mainWindow_) {
        return;
    }
    // We attach to anything that QObject::inherits from View3DInventor. We
    // can't qobject_cast safely without the type being a QObject in the
    // public header, but findChildren<QWidget*> + className check works.
    for (auto* w : mainWindow_->findChildren<QWidget*>()) {
        if (w == nullptr) {
            continue;
        }
        const QString cls = QString::fromLatin1(w->metaObject()->className());
        if (!cls.contains(QStringLiteral("View3DInventor"))) {
            continue;
        }
        // Avoid double-attaching: tag the viewer with a property.
        if (w->property("linuxcadHasViewCluster").toBool()) {
            continue;
        }
        attachToViewer(w);
    }
}

void ViewWidgetsOverlay::attachToViewer(QWidget* viewer)
{
    if (viewer == nullptr) {
        return;
    }
    auto* cluster = new ViewClusterWidget(viewer);
    cluster->show();
    cluster->reposition();
    viewer->setProperty("linuxcadHasViewCluster", true);
    clusters_.append(cluster);
}

} // namespace LinuxCAD
} // namespace Gui
