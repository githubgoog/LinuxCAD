// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>

#include "Ribbon.h"

namespace Gui {
namespace LinuxCAD {

namespace {

// QToolBars whose objectName starts with one of these prefixes are part of
// LinuxCAD's own chrome and must not be folded into the ribbon.
bool isLinuxCadChrome(const QString& objectName)
{
    static const QStringList kPrefixes = {
        QStringLiteral("LinuxCad"),
    };
    for (const auto& p : kPrefixes) {
        if (objectName.startsWith(p)) {
            return true;
        }
    }
    return false;
}

QString humanizeToolBarTitle(const QToolBar* tb)
{
    if (tb == nullptr) {
        return QString();
    }
    const QString title = tb->windowTitle().trimmed();
    if (!title.isEmpty()) {
        return title;
    }
    return tb->objectName();
}

QToolButton* makeRibbonButton(QAction* action, QWidget* parent)
{
    auto* btn = new QToolButton(parent);
    btn->setDefaultAction(action);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setAutoRaise(true);
    btn->setIconSize(QSize(28, 28));
    btn->setProperty("linuxcadRole", QStringLiteral("ribbon-button"));
    btn->setMinimumWidth(56);
    btn->setMaximumWidth(96);
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return btn;
}

} // namespace

Ribbon::Ribbon(QWidget* parent)
    : QToolBar(parent)
{
    setObjectName(QStringLiteral("LinuxCadRibbon"));
    setWindowTitle(tr("LinuxCAD Ribbon"));
    setMovable(false);
    setFloatable(false);
    setProperty("linuxcadRole", QStringLiteral("ribbon"));
    setIconSize(QSize(28, 28));

    scroll_ = new QScrollArea(this);
    scroll_->setObjectName(QStringLiteral("LinuxCadRibbonScroll"));
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_->setProperty("linuxcadRole", QStringLiteral("ribbon-scroll"));

    content_ = new QWidget(scroll_);
    content_->setProperty("linuxcadRole", QStringLiteral("ribbon-content"));
    row_ = new QHBoxLayout(content_);
    row_->setContentsMargins(8, 4, 8, 4);
    row_->setSpacing(0);
    row_->addStretch();

    scroll_->setWidget(content_);
    addWidget(scroll_);

    // Subscribe to workbench activation. FreeCAD signals via boost::signals2;
    // any throw escaping the slot would tear down the application, so we
    // wrap defensively.
    if (auto* app = Gui::Application::Instance) {
        try {
            app->signalActivateWorkbench.connect([this](const char* /*name*/) {
                scheduleRebuild();
            });
        }
        catch (...) {
            // Defensive: never let a signal hookup break the UI.
        }
    }

    // First fill happens once the host MainWindow's toolbars are populated.
    QTimer::singleShot(0, this, &Ribbon::rebuild);
}

Ribbon::~Ribbon() = default;

void Ribbon::scheduleRebuild()
{
    // Workbench activation populates QToolBars asynchronously - run on the
    // next event-loop tick so we observe the populated state.
    QTimer::singleShot(0, this, &Ribbon::rebuild);
}

bool Ribbon::shouldSkipToolBar(QToolBar* tb) const
{
    if (tb == nullptr || tb == this) {
        return true;
    }
    if (isLinuxCadChrome(tb->objectName())) {
        return true;
    }
    // Ignore empty toolbars; they'd render as an empty group label.
    if (tb->actions().isEmpty()) {
        return true;
    }
    // Status-bar toolbars and tiny utility toolbars often have <2 actions.
    int visibleActions = 0;
    for (auto* act : tb->actions()) {
        if (act != nullptr && !act->isSeparator() && act->isVisible()) {
            ++visibleActions;
        }
    }
    return visibleActions == 0;
}

QWidget* Ribbon::buildGroup(QToolBar* sourceToolBar)
{
    auto* group = new QWidget(content_);
    group->setProperty("linuxcadRole", QStringLiteral("ribbon-group"));

    auto* col = new QVBoxLayout(group);
    col->setContentsMargins(6, 0, 6, 0);
    col->setSpacing(2);

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(2);
    col->addLayout(row);

    for (auto* act : sourceToolBar->actions()) {
        if (act == nullptr) {
            continue;
        }
        if (act->isSeparator()) {
            auto* sep = new QFrame(group);
            sep->setFrameShape(QFrame::VLine);
            sep->setFrameShadow(QFrame::Plain);
            sep->setProperty("linuxcadRole", QStringLiteral("ribbon-inner-sep"));
            row->addWidget(sep);
            continue;
        }
        if (!act->isVisible()) {
            continue;
        }
        row->addWidget(makeRibbonButton(act, group));
    }

    auto* label = new QLabel(humanizeToolBarTitle(sourceToolBar), group);
    label->setAlignment(Qt::AlignHCenter);
    label->setProperty("linuxcadRole", QStringLiteral("ribbon-group-label"));
    col->addWidget(label);

    return group;
}

void Ribbon::clearGroups()
{
    if (row_ == nullptr) {
        return;
    }
    while (row_->count() > 0) {
        QLayoutItem* item = row_->takeAt(0);
        if (item == nullptr) {
            continue;
        }
        if (auto* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

void Ribbon::rebuild()
{
    if (row_ == nullptr || content_ == nullptr) {
        return;
    }

    auto* mw = Gui::getMainWindow();
    if (mw == nullptr) {
        return;
    }

    clearGroups();

    bool addedAny = false;
    const auto toolBars = mw->findChildren<QToolBar*>();
    for (auto* tb : toolBars) {
        if (shouldSkipToolBar(tb)) {
            continue;
        }

        // Hide the original toolbar so its icons don't appear above the canvas
        // alongside the ribbon. The actions stay alive and are reachable both
        // through the ribbon and any keyboard shortcut FreeCAD assigned them.
        if (tb->isVisible()) {
            tb->setVisible(false);
        }
        if (auto* viewAct = tb->toggleViewAction()) {
            viewAct->setVisible(false);
        }

        if (auto* group = buildGroup(tb)) {
            row_->addWidget(group);

            auto* divider = new QFrame(content_);
            divider->setFrameShape(QFrame::VLine);
            divider->setFrameShadow(QFrame::Plain);
            divider->setProperty("linuxcadRole", QStringLiteral("ribbon-divider"));
            row_->addWidget(divider);

            addedAny = true;
        }
    }

    if (addedAny) {
        // Keep a trailing stretch so groups left-align nicely.
        row_->addStretch();
    }
    else {
        // Empty workbench placeholder so the bar height stays stable.
        auto* placeholder = new QLabel(tr("No tools for this workbench"), content_);
        placeholder->setProperty("linuxcadRole", QStringLiteral("ribbon-empty"));
        placeholder->setAlignment(Qt::AlignCenter);
        row_->addWidget(placeholder);
        row_->addStretch();
    }
}

} // namespace LinuxCAD
} // namespace Gui

