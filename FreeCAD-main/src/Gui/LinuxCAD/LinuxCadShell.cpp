// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#endif

#include <Base/Console.h>
#include <Gui/MainWindow.h>

#include "LinuxCadShell.h"
#include "TopBar.h"
#include "ProjectManagerDock.h"
#include "WelcomeScreen.h"
#include "ProjectManager.h"
#include "Theme.h"
#include "CommandPalette.h"

namespace Gui {
namespace LinuxCAD {

namespace {
Shell* g_instance = nullptr;
}

Shell* Shell::instance()
{
    return g_instance;
}

Shell* shell()
{
    return g_instance;
}

void install(Gui::MainWindow* mw)
{
    if (!mw) {
        return;
    }
    if (g_instance) {
        // Already installed.
        return;
    }

    Base::Console().log("LinuxCAD: installing shell on MainWindow\n");

    g_instance = new Shell();
    g_instance->mainWindow_ = mw;

    // Apply theme first so all subsequent widgets pick it up.
    g_instance->theme_ = new Theme(mw);
    g_instance->theme_->applyDefault();

    // Project model + manager service.
    g_instance->projectManager_ = new ProjectManager(mw);

    // The new top bar replaces FreeCAD's QMenuBar visibility.
    g_instance->topBar_ = new TopBar(mw);
    mw->addToolBar(Qt::TopToolBarArea, g_instance->topBar_);
    g_instance->topBar_->setObjectName(QStringLiteral("LinuxCadTopBar"));

    // Project manager dock on the left.
    g_instance->projectDock_ = new ProjectManagerDock(g_instance->projectManager_, mw);
    mw->addDockWidget(Qt::LeftDockWidgetArea, g_instance->projectDock_);

    // Welcome screen overlay (shown when no document is open).
    g_instance->welcomeScreen_ = new WelcomeScreen(g_instance->projectManager_, mw);

    // Command palette (Ctrl+K).
    g_instance->commandPalette_ = new CommandPalette(mw);

    // Hide FreeCAD's classic menu bar by default. A "Show classic menu" preference
    // is offered in the LinuxCAD preferences pane as an escape hatch.
    if (auto* mb = mw->menuBar()) {
        mb->setVisible(false);
    }

    Base::Console().log("LinuxCAD: shell installed\n");
}

} // namespace LinuxCAD
} // namespace Gui
