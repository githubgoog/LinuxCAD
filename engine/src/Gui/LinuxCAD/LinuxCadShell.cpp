// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QDockWidget>
#include <QMenuBar>
#include <QPointer>
#include <QSettings>
#include <QShortcut>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>

#include "AI/Consent.h"
#include "AI/GhostToast.h"
#include "AI/Provider.h"
#include "AI/SuggestionEngine.h"
#include "CheatSheet.h"
#include "CommandPalette.h"
#include "LinuxCadShell.h"
#include "NaviCubeDefaults.h"
#include "ProjectManager.h"
#include "ProjectManagerDock.h"
#include "Ribbon.h"
#include "Shortcuts.h"
#include "TopBar.h"
#include "Theme.h"
#include "ViewWidgetsOverlay.h"
#include "WelcomeScreen.h"

namespace Gui {
namespace LinuxCAD {

namespace {
Shell* g_instance = nullptr;

void enableDockPreferenceGroup(const char* group, bool enabled)
{
    auto handle = App::GetApplication().GetUserParameter()
                      .GetGroup("BaseApp")
                      ->GetGroup("Preferences")
                      ->GetGroup("DockWindows")
                      ->GetGroup(group);
    handle->SetBool("Enabled", enabled);
}

void applyDefaultNavigationStyleOnce()
{
    QSettings s;
    const QString flagKey = QStringLiteral("LinuxCAD/NavStyleAppliedV1");
    if (s.value(flagKey, false).toBool()) {
        return;
    }
    auto view = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");
    if (view) {
        view->SetASCII("NavigationStyle", "Gui::LinuxCadNavigationStyle");
    }
    s.setValue(flagKey, true);
    Base::Console().log("LinuxCAD: default navigation style set to "
                        "Gui::LinuxCadNavigationStyle\n");
}

QDockWidget* findDockByObjectName(Gui::MainWindow* mw, const char* widgetObjectName)
{
    if (mw == nullptr) {
        return nullptr;
    }
    const QString needle = QString::fromLatin1(widgetObjectName);
    for (auto* dock : mw->findChildren<QDockWidget*>()) {
        if (dock == nullptr) {
            continue;
        }
        if (dock->objectName() == needle) {
            return dock;
        }
        if (auto* w = dock->widget()) {
            if (w->objectName() == needle) {
                return dock;
            }
        }
    }
    return nullptr;
}

void brandDock(QDockWidget* dock, const QString& title, const QString& role)
{
    if (dock == nullptr) {
        return;
    }
    if (!title.isEmpty()) {
        dock->setWindowTitle(title);
    }
    if (!role.isEmpty()) {
        dock->setProperty("linuxcadRole", role);
    }
}

void hideClassicTopToolbars(Gui::MainWindow* mw)
{
    if (mw == nullptr) {
        return;
    }
    for (auto* tb : mw->findChildren<QToolBar*>()) {
        if (tb == nullptr) {
            continue;
        }
        const QString name = tb->objectName();
        if (name.startsWith(QStringLiteral("LinuxCad"))) {
            continue;
        }
        tb->setVisible(false);
        if (auto* viewAct = tb->toggleViewAction()) {
            viewAct->setVisible(false);
        }
    }
}

bool anyDocumentOpen()
{
    return !App::GetApplication().getDocuments().empty();
}

} // namespace

Shell* Shell::instance()
{
    return g_instance;
}

Shell* shell()
{
    return g_instance;
}

void Shell::showCheatSheet()
{
    if (cheatSheet_ != nullptr) {
        cheatSheet_->showOverlay();
    }
}

void Shell::newSketchInteractive()
{
    // Implemented in Pillar 5 - delegated through ProjectManager so the
    // sketch-first flow can wrap document creation in a transaction.
    if (projectManager_ != nullptr) {
        projectManager_->newSketchInteractive(mainWindow_);
    }
}

void install(Gui::MainWindow* mw)
{
    if (mw == nullptr) {
        return;
    }
    if (g_instance != nullptr) {
        return; // idempotent
    }

    Base::Console().log("LinuxCAD: installing shell on MainWindow\n");

    g_instance = new Shell();
    g_instance->mainWindow_ = mw;

    // --- Theme -----------------------------------------------------------
    g_instance->theme_ = new Theme(mw);
    g_instance->theme_->applyDefault();

    // --- Project model + manager ----------------------------------------
    g_instance->projectManager_ = new ProjectManager(mw);

    // --- TopBar (row 1) -------------------------------------------------
    g_instance->topBar_ = new TopBar(mw);
    mw->addToolBar(Qt::TopToolBarArea, g_instance->topBar_);
    g_instance->topBar_->setObjectName(QStringLiteral("LinuxCadTopBar"));

    // --- Ribbon (row 2) -------------------------------------------------
    mw->addToolBarBreak(Qt::TopToolBarArea);
    g_instance->ribbon_ = new Ribbon(mw);
    mw->addToolBar(Qt::TopToolBarArea, g_instance->ribbon_);

    // --- Project manager dock (left) ------------------------------------
    g_instance->projectDock_ = new ProjectManagerDock(g_instance->projectManager_, mw);
    g_instance->projectDock_->setObjectName(QStringLiteral("LinuxCadProjectDock"));
    g_instance->projectDock_->setProperty("linuxcadRole", QStringLiteral("project-dock"));
    mw->addDockWidget(Qt::LeftDockWidgetArea, g_instance->projectDock_);

    // --- Force FreeCAD's Combo / Property docks to be present and themed
    // We enable the preferences first (cheap, idempotent), then ask the
    // MainWindow to materialize them. Combo View by default lives on the
    // left and contains the Model tree; Property View we force on the right.
    enableDockPreferenceGroup("ComboView", true);
    enableDockPreferenceGroup("PropertyView", true);
    mw->initDockWindows(true);

    if (auto* combo = findDockByObjectName(mw, "Model")) {
        mw->addDockWidget(Qt::LeftDockWidgetArea, combo);
        brandDock(combo, QObject::tr("Model"), QStringLiteral("tree-dock"));
        // Tabify the Project pane next to the Model tab so they share space.
        mw->tabifyDockWidget(combo, g_instance->projectDock_);
        combo->raise();
    }
    if (auto* props = findDockByObjectName(mw, "Property view")) {
        mw->addDockWidget(Qt::RightDockWidgetArea, props);
        brandDock(props, QObject::tr("Properties"), QStringLiteral("props-dock"));
    }

    // --- Welcome screen overlay -----------------------------------------
    g_instance->welcomeScreen_ = new WelcomeScreen(g_instance->projectManager_, mw);

    // --- Command palette (Ctrl+K) ---------------------------------------
    g_instance->commandPalette_ = new CommandPalette(mw);

    // --- Cheat-sheet overlay (Pillar 4) ---------------------------------
    g_instance->cheatSheet_ = new CheatSheet(g_instance->commandPalette_, mw);

    // --- Curated keyboard shortcuts (Pillar 4) --------------------------
    Shortcuts::applyDefaults();

    // Global ? and Ctrl+/ shortcuts for the cheat sheet, anywhere in the app.
    auto* scQuestion = new QShortcut(QKeySequence(QStringLiteral("?")), mw);
    scQuestion->setContext(Qt::ApplicationShortcut);
    QObject::connect(scQuestion, &QShortcut::activated, mw, []() {
        if (auto* sh = Shell::instance()) {
            sh->showCheatSheet();
        }
    });
    auto* scSlash = new QShortcut(QKeySequence(QStringLiteral("Ctrl+/")), mw);
    scSlash->setContext(Qt::ApplicationShortcut);
    QObject::connect(scSlash, &QShortcut::activated, mw, []() {
        if (auto* sh = Shell::instance()) {
            sh->showCheatSheet();
        }
    });

    // --- NaviCube defaults (Pillar 2) -----------------------------------
    NaviCubeDefaults::applyOnce();

    // --- Default navigation style (Pillar 3) ----------------------------
    applyDefaultNavigationStyleOnce();

    // --- View widgets overlay (Pillar 2) --------------------------------
    g_instance->viewWidgets_ = new ViewWidgetsOverlay(mw);

    // --- AI subsystem (Pillar 7) ----------------------------------------
    g_instance->aiProvider_ = Provider::createFromSettings(mw);
    g_instance->ghostToast_ = new GhostToast(mw);
    g_instance->suggestionEngine_ = new SuggestionEngine(g_instance->aiProvider_,
                                                          g_instance->ghostToast_,
                                                          mw);
    QObject::connect(
        g_instance->suggestionEngine_, &SuggestionEngine::stateChanged,
        g_instance->topBar_, [](SuggestionEngine::State s) {
            if (auto* sh = Shell::instance()) {
                if (auto* tb = sh->topBar()) {
                    tb->onAiStateChanged(static_cast<int>(s));
                }
            }
        });
    g_instance->suggestionEngine_->start();
    // Initial badge sync.
    if (g_instance->topBar_ != nullptr && g_instance->suggestionEngine_ != nullptr) {
        g_instance->topBar_->onAiStateChanged(
            static_cast<int>(g_instance->suggestionEngine_->state()));
    }

    // First-run consent prompt - deferred so it shows over a fully painted
    // main window. If the user accepts, recreate the suggestion engine so
    // it picks up the freshly enabled state.
    QTimer::singleShot(1500, mw, [mw]() {
        Consent::promptIfNeeded(mw);
        if (auto* sh = Shell::instance()) {
            if (sh->suggestionEngine() != nullptr) {
                sh->suggestionEngine()->start();
                if (sh->topBar() != nullptr) {
                    sh->topBar()->onAiStateChanged(
                        static_cast<int>(sh->suggestionEngine()->state()));
                }
            }
        }
    });

    // --- Hide FreeCAD's classic chrome ----------------------------------
    if (auto* mb = mw->menuBar()) {
        mb->setVisible(false);
    }
    QTimer::singleShot(0, mw, []() {
        if (auto* sh = Shell::instance()) {
            hideClassicTopToolbars(sh->mainWindow());
            // Keep our own top bars visible.
            if (auto* tb = sh->topBar()) {
                tb->setVisible(true);
            }
            if (auto* rb = sh->ribbon()) {
                rb->setVisible(true);
                rb->rebuild();
            }
        }
    });

    // --- Welcome show/hide on document open/close -----------------------
    auto refreshWelcome = []() {
        if (auto* sh = Shell::instance()) {
            if (auto* ws = sh->welcomeScreen()) {
                if (anyDocumentOpen()) {
                    ws->hide();
                }
                else {
                    ws->showCentered();
                }
            }
        }
    };
    try {
        App::GetApplication().signalNewDocument.connect(
            [refreshWelcome](const App::Document&, bool) { refreshWelcome(); });
        App::GetApplication().signalDeletedDocument.connect(
            [refreshWelcome]() {
                QTimer::singleShot(0, [refreshWelcome]() { refreshWelcome(); });
            });
    }
    catch (...) {
        // Defensive: never let a signal hookup break the UI.
    }
    QTimer::singleShot(0, mw, refreshWelcome);

    Base::Console().log("LinuxCAD: shell installed\n");
}

} // namespace LinuxCAD
} // namespace Gui
