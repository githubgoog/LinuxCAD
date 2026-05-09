// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QIcon>
#include <QFont>
#include <QDockWidget>
#include <QMenuBar>
#include <QObject>
#include <QSettings>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/WorkbenchManager.h>

#include <string>

#include "AI/Consent.h"
#include "AI/GhostToast.h"
#include "AI/Provider.h"
#include "AI/SuggestionEngine.h"
#include "CommandPalette.h"
#include "FirstRunWizard.h"
#include "LinuxCadStart.h"
#include "LinuxCadShell.h"
#include "NaviCubeDefaults.h"
#include "Ribbon.h"
#include "Shortcuts.h"
#include "TopBar.h"
#include "Theme.h"
#include "ViewWidgetsOverlay.h"
#include "WorkbenchDropdownButton.h"

namespace Gui {
namespace LinuxCAD {

namespace {
Shell* g_instance = nullptr;

static void applyApplicationFont()
{
    if (auto* app = QApplication::instance()) {
        Q_UNUSED(app);
        QFont f(QStringLiteral("Inter"));
        f.setStyleHint(QFont::SansSerif, QFont::PreferAntialias);
        f.setPixelSize(13);
        f.setWeight(QFont::Normal);
        QApplication::setFont(f);
        if (auto* mw = Gui::getMainWindow()) {
            mw->setFont(f);
            if (auto* mb = mw->menuBar()) {
                mb->setFont(f);
            }
            if (auto* sb = mw->statusBar()) {
                sb->setFont(f);
            }
        }
    }
}

void enableDockPreferenceGroup(const char* group, bool enabled)
{
    auto handle = App::GetApplication().GetUserParameter()
                      .GetGroup("BaseApp")
                      ->GetGroup("Preferences")
                      ->GetGroup("DockWindows")
                      ->GetGroup(group);
    handle->SetBool("Enabled", enabled);
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

bool linuxcadPrefersDarkChrome()
{
    QSettings qs;
    const QString id = Theme::normalizeThemeId(
        qs.value(QStringLiteral("LinuxCAD/Theme"), Theme::defaultThemeId()).toString());
    return id != QStringLiteral("light-amber");
}

bool linuxcadUsesSystemTheme()
{
    QSettings qs;
    return Theme::normalizeThemeId(
               qs.value(QStringLiteral("LinuxCAD/Theme"), Theme::defaultThemeId()).toString())
           == QStringLiteral("system");
}

QString dockTintStyleSheet(bool dark)
{
    if (dark) {
        return QStringLiteral(
            "QAbstractItemView {"
            "  background-color: #14171C;"
            "  color: #ECEEF1;"
            "  alternate-background-color: #1B1F25;"
            "  selection-background-color: #2C323B;"
            "  selection-color: #ECEEF1;"
            "  border: 1px solid #2C323B;"
            "  outline: none;"
            "}"
            "QSplitter::handle { background: #2C323B; }"
            "QPlainTextEdit, QTextEdit, QTextBrowser {"
            "  background-color: #14171C;"
            "  color: #ECEEF1;"
            "  border: 1px solid #2C323B;"
            "}"
            "QTabWidget::pane { background: #14171C; border: 1px solid #2C323B; }"
            "QTabBar::tab { background: #1B1F25; color: #A8AEB8; border: 1px solid #2C323B; padding: "
            "4px "
            "12px; }"
            "QTabBar::tab:selected { background: #232830; color: #ECEEF1; }"
            "QScrollBar:vertical { background: #14171C; width: 10px; }"
            "QScrollBar::handle:vertical { background: #3A4150; min-height: 24px; }"
            /* Property / Report inner hosts often nest QScrollArea + plain QWidget. */
            "QScrollArea, QScrollArea > QWidget > QWidget {"
            "  background-color: #14171C;"
            "}");
    }

    return QStringLiteral(
        "QAbstractItemView {"
        "  background-color: #FFFFFF;"
        "  color: #1A1F26;"
        "  alternate-background-color: #F4F6F8;"
        "  selection-background-color: #E8F1FB;"
        "  selection-color: #1A1F26;"
        "  border: 1px solid #DCE0E5;"
        "  outline: none;"
        "}"
        "QSplitter::handle { background: #DCE0E5; }"
        "QPlainTextEdit, QTextEdit, QTextBrowser {"
        "  background-color: #FFFFFF;"
        "  color: #1A1F26;"
        "  border: 1px solid #DCE0E5;"
        "}"
        "QTabWidget::pane { background: #FAFBFC; border: 1px solid #DCE0E5; }"
        "QTabBar::tab { background: #F1F3F5; color: #4B5563; border: 1px solid #DCE0E5; padding: 4px "
        "12px; "
        "}"
        "QTabBar::tab:selected { background: #FFFFFF; color: #1A1F26; }"
        "QScrollBar:vertical { background: #FAFBFC; width: 10px; }"
        "QScrollBar::handle:vertical { background: #C5CCD6; min-height: 24px; }");
}

void applyDockTint(QDockWidget* dock)
{
    if (dock == nullptr) {
        return;
    }
    const QString rTree = QStringLiteral("tree-dock");
    const QString rProps = QStringLiteral("props-dock");
    const QString rReport = QStringLiteral("report-dock");
    const QString rProj = QStringLiteral("project-dock");
    const QVariant roleVar = dock->property("linuxcadRole");
    if (!roleVar.isValid()) {
        return;
    }
    const QString roleStr = roleVar.toString();
    if (roleStr != rTree && roleStr != rProps && roleStr != rReport && roleStr != rProj) {
        return;
    }
    if (linuxcadUsesSystemTheme()) {
        dock->setStyleSheet(QString());
        return;
    }
    dock->setStyleSheet(dockTintStyleSheet(linuxcadPrefersDarkChrome()));
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
        applyDockTint(dock);
    }
}

bool workbenchLooksPlaceholder(Gui::Application* ga, const std::string& internalName)
{
    if (ga == nullptr) {
        return true;
    }

    QString key = QString::fromStdString(internalName);
    key = key.trimmed();
    if (key.isEmpty()) {
        return true;
    }
    if (key.compare(QLatin1String("NoneWorkbench"), Qt::CaseInsensitive) == 0) {
        return true;
    }

    const QString menu = ga->workbenchMenuText(key);
    return menu.compare(QLatin1String("<none>"), Qt::CaseInsensitive) == 0;
}

bool tryActivateWorkbench(Gui::Application* ga, const char* name)
{
    if (ga == nullptr || name == nullptr || *name == '\0') {
        return false;
    }
    try {
        ga->activateWorkbench(name);
        return true;
    }
    catch (...) {
        return false;
    }
}

QIcon linuxCadWindowIcon()
{
    const QIcon icon(QStringLiteral(":/linuxcad/branding/glyph.svg"));
    return icon;
}

void activateLinuxCadStartupWorkbench()
{
    auto* ga = Gui::Application::Instance;
    if (ga == nullptr) {
        return;
    }

    std::string current;
    try {
        if (auto* mgr = Gui::WorkbenchManager::instance()) {
            current = mgr->activeName();
        }
    }
    catch (...) {
        current.clear();
    }

    // Do not override a deliberate non-placeholder workbench.
    if (!workbenchLooksPlaceholder(ga, current)) {
        return;
    }

    const std::string startCfg = App::Application::Config()["StartWorkbench"];
    std::string configHolder;
    if (!startCfg.empty() && startCfg != "NoneWorkbench") {
        configHolder = startCfg;
        if (tryActivateWorkbench(ga, configHolder.c_str())) {
            return;
        }
    }

    static const char* kFallbackWorkbenches[] =
        {"PartDesignWorkbench",
         "SketcherWorkbench",
         "PartWorkbench"};
    for (const char* cand : kFallbackWorkbenches) {
        if (cand != nullptr && tryActivateWorkbench(ga, cand)) {
            return;
        }
    }
}

/// Sync shell chrome after workbench/toolbars change (also safe early at startup).
void polishLinuxCadShellUi(Gui::MainWindow* mw)
{
    if (mw == nullptr) {
        return;
    }

    Shell* sh = Shell::instance();
    if (sh == nullptr) {
        return;
    }

    if (auto* tb = sh->topBar()) {
        tb->setRibbonRowInteractive(true);
        tb->setVisible(true);
        if (auto* dd = tb->findChild<WorkbenchDropdownButton*>(QStringLiteral("workbench-dropdown"))) {
            dd->syncToActive();
        }
    }
    if (auto* rb = sh->ribbon()) {
        rb->setVisible(true);
        rb->scheduleRebuild();
    }
    refreshDockChromeTint();
}

} // unnamed namespace helpers

Shell* Shell::instance()
{
    return g_instance;
}

Shell* shell()
{
    return g_instance;
}

void refreshDockChromeTint()
{
    if (g_instance == nullptr || g_instance->mainWindow() == nullptr) {
        return;
    }
    for (auto* d : g_instance->mainWindow()->findChildren<QDockWidget*>()) {
        applyDockTint(d);
    }
}

void Shell::reloadAiProvider()
{
    Provider* fresh = Provider::createFromSettings(mainWindow_);

    if (suggestionEngine_) {
        suggestionEngine_->setProvider(fresh);
    }
    if (aiProvider_ && aiProvider_ != fresh) {
        aiProvider_->deleteLater();
    }
    aiProvider_ = fresh;

    if (topBar_ != nullptr && suggestionEngine_ != nullptr) {
        topBar_->onAiStateChanged(static_cast<int>(suggestionEngine_->state()));
    }
}

bool Shell::anyDocumentOpen() const
{
    try {
        return !App::GetApplication().getDocuments().empty();
    }
    catch (...) {
        return false;
    }
}

void Shell::ensureStartViewInstalled()
{
    if (mainWindow_ == nullptr) {
        return;
    }
    if (startView_ == nullptr) {
        startView_ = new LinuxCadStart(mainWindow_, mainWindow_);
        startView_->hide();
    }
}

void Shell::refreshStartViewVisibility()
{
    if (mainWindow_ == nullptr) {
        return;
    }

    ensureStartViewInstalled();
    if (startView_ == nullptr) {
        return;
    }

    QWidget* host = mainWindow_->centralWidget();
    if (host == nullptr) {
        startView_->hide();
        return;
    }

    startView_->setHostWidget(host);
    const bool shouldShow = !anyDocumentOpen();
    if (shouldShow) {
        startView_->refreshRecents();
        startView_->syncToHost();
        startView_->show();
        startView_->raise();
    }
    else {
        startView_->hide();
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
    applyApplicationFont();
    const QIcon linuxcadIcon = linuxCadWindowIcon();
    if (!linuxcadIcon.isNull()) {
        mw->setWindowIcon(linuxcadIcon);
        QApplication::setWindowIcon(linuxcadIcon);
    }

    // --- Theme -----------------------------------------------------------
    g_instance->theme_ = new Theme(mw);
    g_instance->theme_->applyDefault();

    // --- TopBar (row 1) -------------------------------------------------
    g_instance->topBar_ = new TopBar(mw);
    mw->addToolBar(Qt::TopToolBarArea, g_instance->topBar_);
    g_instance->topBar_->setObjectName(QStringLiteral("LinuxCadTopBar"));
    // Keep the native menu bar (File/Edit/...) visible per UX direction.
    if (auto* mb = mw->menuBar()) {
        mb->setVisible(true);
    }

    // --- Ribbon (QWidget; Wave 2E TopBar embeds via setRibbonBody) -------
    // Construct without a parent and let TopBar::setRibbonBody reparent it
    // onto the row-2 holder. That keeps the ribbon's lifetime owned by the
    // TopBar and prevents Qt from inserting it as a separate top-level.
    g_instance->ribbon_ = new Ribbon();
    g_instance->topBar_->setRibbonBody(g_instance->ribbon_);
    if (g_instance->topBar_ != nullptr && g_instance->ribbon_ != nullptr) {
        if (auto* dd = g_instance->topBar_->findChild<WorkbenchDropdownButton*>(
                QStringLiteral("workbench-dropdown"))) {
            QObject::connect(dd,
                             &WorkbenchDropdownButton::workbenchActivated,
                             mw,
                             [mw]() {
                                 if (auto* sh = Shell::instance()) {
                                     if (auto* rb = sh->ribbon()) {
                                         rb->scheduleRebuild();
                                         QTimer::singleShot(220, mw, [mw]() {
                                             if (auto* shell = Shell::instance()) {
                                                 if (auto* ribbon = shell->ribbon()) {
                                                     ribbon->scheduleRebuild();
                                                 }
                                             }
                                         });
                                         QTimer::singleShot(500, mw, [mw]() {
                                             if (auto* shell = Shell::instance()) {
                                                 if (auto* ribbon = shell->ribbon()) {
                                                     ribbon->scheduleRebuild();
                                                 }
                                             }
                                         });
                                     }
                                 }
                            });
        }
    }

    // --- Start surface overlay (empty-document state) -------------------
    g_instance->ensureStartViewInstalled();
    g_instance->refreshStartViewVisibility();

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
        combo->raise();
    }
    if (auto* props = findDockByObjectName(mw, "Property view")) {
        mw->addDockWidget(Qt::RightDockWidgetArea, props);
        brandDock(props, QObject::tr("Properties"), QStringLiteral("props-dock"));
    }
    if (auto* report = findDockByObjectName(mw, "Report view")) {
        brandDock(report, QObject::tr("Report View"), QStringLiteral("report-dock"));
    }

    // --- Command palette (Ctrl+K) ---------------------------------------
    g_instance->commandPalette_ = new CommandPalette(mw);

    // --- Curated keyboard shortcuts (Pillar 4) --------------------------
    Shortcuts::applyDefaults();

    // --- NaviCube defaults (Pillar 2) -----------------------------------
    NaviCubeDefaults::applyOnce();

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

    // Apply sensible first-run defaults silently (dark theme, metric units,
    // LinuxCAD navigation, AI mock disabled). The full setup wizard remains
    // accessible via the LinuxCAD logo menu -> "Re-run setup wizard".
    FirstRunWizard::applySilentDefaults();

    // One-time: prefer a maximized main window on first launch after this
    // update (user can restore down; MainWindow persists thereafter).
    {
        QSettings qs;
        const QString kMax = QStringLiteral("LinuxCAD/PreferMaximizedBootstrapV1");
        if (!qs.value(kMax, false).toBool()) {
            qs.setValue(kMax, true);
            try {
                auto h = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/MainWindow");
                h->SetBool("Maximized", true);
            }
            catch (...) {
            }
        }
    }

    // First-run consent prompt - deferred so it shows over a fully painted
    // main window. If the user accepts, recreate the suggestion engine so
    // it picks up the freshly enabled state.
    QTimer::singleShot(4600, mw, [mw]() {
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

    try {
        if (Gui::Application::Instance != nullptr) {
            Gui::Application::Instance->signalActivateWorkbench.connect(
                [mw](const char*) {
                    QTimer::singleShot(
                        0,
                        mw,
                        [mw]() {
                            polishLinuxCadShellUi(mw);
                        });
                });
        }
    }
    catch (...) {
        // Defensive
    }

    // --- Refresh chrome when documents appear / disappear ----------------
    auto refreshChromeOnDocs = []() {
        if (auto* sh = Shell::instance()) {
            polishLinuxCadShellUi(sh->mainWindow());
            sh->refreshStartViewVisibility();
            if (auto* tb = sh->topBar()) {
                // Ribbon hosts workbench tools; keep it interactive without a document
                // so newcomers still see Sketcher / core actions as expected.
                tb->setRibbonRowInteractive(true);
            }
        }
    };
    try {
        App::GetApplication().signalNewDocument.connect(
            [refreshChromeOnDocs](const App::Document&, bool) { refreshChromeOnDocs(); });
        App::GetApplication().signalDeletedDocument.connect(
            [refreshChromeOnDocs]() {
                QTimer::singleShot(0, [refreshChromeOnDocs]() { refreshChromeOnDocs(); });
            });
    }
    catch (...) {
        // Defensive: never let a signal hookup break the UI.
    }
    QTimer::singleShot(0, mw, refreshChromeOnDocs);

    // FreeCAD's startup activates NoneWorkbench ("<none>") briefly; retries cover
    // late-loaded modules. Always polish chrome so dropdown/ribbon catches up.
    auto kickWorkbenchAndChrome = [mw]() {
        activateLinuxCadStartupWorkbench();
        polishLinuxCadShellUi(mw);
    };
    QTimer::singleShot(0, mw, kickWorkbenchAndChrome);
    QTimer::singleShot(100, mw, kickWorkbenchAndChrome);
    QTimer::singleShot(500, mw, kickWorkbenchAndChrome);
    QTimer::singleShot(2000, mw, kickWorkbenchAndChrome);
    QTimer::singleShot(4500, mw, kickWorkbenchAndChrome);

    Base::Console().log("LinuxCAD: shell installed\n");
}

} // namespace LinuxCAD
} // namespace Gui
