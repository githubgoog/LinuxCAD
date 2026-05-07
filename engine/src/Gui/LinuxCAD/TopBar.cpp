// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QPixmap>
#include <QSettings>
#include <QShortcut>
#include <QSize>
#include <QSizePolicy>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>

#include "AI/ConfigureAiDialog.h"
#include "AI/Provider.h"
#include "CommandPalette.h"
#include "FirstRunWizard.h"
#include "LinuxCadShell.h"
#include "ProjectManager.h"
#include "Ribbon.h"
#include "TopBar.h"
#include "WorkbenchDropdownButton.h"

namespace Gui {
namespace LinuxCAD {

TopBar::TopBar(QWidget* parent)
    : QToolBar(parent)
{
    setObjectName(QStringLiteral("LinuxCadTopBar"));
    setWindowTitle(tr("LinuxCAD"));
    setMovable(false);
    setFloatable(false);
    setIconSize(QSize(18, 18));
    setProperty("linuxcadRole", QStringLiteral("topbar"));

    buildLayout();

    // Global Ctrl+K shortcut for the command palette. Parented to the
    // toolbar (this) so the lifetime tracks the TopBar.
    auto* sc = new QShortcut(QKeySequence(QStringLiteral("Ctrl+K")), this);
    connect(sc, &QShortcut::activated, this, &TopBar::onCommandPaletteRequested);

    // Ctrl+/ opens shortcuts view (folded into the palette via "?" trigger).
    auto* scShortcuts = new QShortcut(QKeySequence(QStringLiteral("Ctrl+/")), this);
    connect(scShortcuts, &QShortcut::activated, [this]() {
        if (auto* sh = Shell::instance(); sh && sh->commandPalette()) {
            sh->commandPalette()->showPalette(QStringLiteral("?"));
        }
    });
}

TopBar::~TopBar() = default;

void TopBar::buildLayout()
{
    // ------------------------------------------------------------------
    // Outer shell: a single QWidget that fills the toolbar and hosts the
    // two rows in a QVBoxLayout. We do NOT scatter many addWidget() calls
    // on the toolbar itself; the toolbar gets exactly one addWidget(outer_).
    // ------------------------------------------------------------------
    outer_ = new QWidget(this);
    outer_->setObjectName(QStringLiteral("topbar-outer"));
    outer_->setProperty("linuxcadRole", QStringLiteral("topbar-outer"));
    outer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    outerLayout_ = new QVBoxLayout(outer_);
    outerLayout_->setContentsMargins(2, 4, 2, 4);
    outerLayout_->setSpacing(2);

    // ------------------------------------------------------------------
    // Row 1: logo / workbench dropdown / quick search (stretches) /
    //        palette / spacer / undo / redo / ai badge / user.
    // ------------------------------------------------------------------
    row1_ = new QWidget(outer_);
    row1_->setObjectName(QStringLiteral("topbar-row1"));
    row1_->setProperty("linuxcadRole", QStringLiteral("topbar-row1"));
    auto* row1Layout = new QHBoxLayout(row1_);
    row1Layout->setContentsMargins(0, 0, 0, 0);
    row1Layout->setSpacing(6);

    // --- Logo button ----------------------------------------------------
    logoButton_ = new QToolButton(row1_);
    logoButton_->setObjectName(QStringLiteral("topbar-logo"));
    logoButton_->setProperty("linuxcadRole", QStringLiteral("topbar-logo"));
    logoButton_->setPopupMode(QToolButton::InstantPopup);
    logoButton_->setAutoRaise(true);
    logoButton_->setToolTip(tr("LinuxCAD - project actions"));
    {
        const QPixmap wordmark(QStringLiteral(":/linuxcad/branding/wordmark.svg"));
        if (!wordmark.isNull()) {
            logoButton_->setIcon(QIcon(wordmark));
            logoButton_->setIconSize(QSize(120, 28));
        }
        else {
            logoButton_->setText(tr("LinuxCAD"));
        }
    }
    logoButton_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    buildLogoMenu();
    row1Layout->addWidget(logoButton_);

    // --- Workbench dropdown ---------------------------------------------
    workbenchDropdown_ = new WorkbenchDropdownButton(row1_);
    row1Layout->addWidget(workbenchDropdown_);

    // --- Quick search / command palette --------------------------------
    quickSearch_ = new QLineEdit(row1_);
    quickSearch_->setObjectName(QStringLiteral("cmd-search"));
    quickSearch_->setPlaceholderText(tr("Search commands  (Ctrl+K)"));
    quickSearch_->setMinimumWidth(280);
    quickSearch_->setClearButtonEnabled(true);
    quickSearch_->setProperty("linuxcadRole", QStringLiteral("cmd-search"));
    row1Layout->addWidget(quickSearch_, /*stretch*/ 1);
    connect(quickSearch_, &QLineEdit::textEdited,    this, &TopBar::onQuickSearchEdited);
    connect(quickSearch_, &QLineEdit::returnPressed, this, &TopBar::onCommandPaletteRequested);

    // --- Palette button -------------------------------------------------
    paletteButton_ = makeIconButton("command", tr("Open command palette (Ctrl+K)"),
                                    SLOT(onCommandPaletteRequested()));
    row1Layout->addWidget(paletteButton_);

    // --- Spacer pushes right-side cluster to the end -------------------
    auto* spacer = new QWidget(row1_);
    spacer->setObjectName(QStringLiteral("topbar-spacer"));
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    row1Layout->addWidget(spacer);

    // --- Undo / Redo ----------------------------------------------------
    undoButton_ = makeIconButton("edit-undo", tr("Undo (Ctrl+Z)"), SLOT(onUndo()));
    redoButton_ = makeIconButton("edit-redo", tr("Redo (Ctrl+Shift+Z)"), SLOT(onRedo()));
    row1Layout->addWidget(undoButton_);
    row1Layout->addWidget(redoButton_);

    // --- AI badge -------------------------------------------------------
    aiBadge_ = new QToolButton(row1_);
    aiBadge_->setObjectName(QStringLiteral("topbar-ai-badge"));
    aiBadge_->setText(tr("AI"));
    aiBadge_->setProperty("linuxcadRole", QStringLiteral("ai-status"));
    aiBadge_->setProperty("state", QStringLiteral("disabled"));
    aiBadge_->setToolTip(tr("AI assistant - click to configure"));
    aiBadge_->setAutoRaise(false);
    row1Layout->addWidget(aiBadge_);
    connect(aiBadge_, &QToolButton::clicked, this, &TopBar::onAiBadgeClicked);

    // --- User area ------------------------------------------------------
    userButton_ = makeIconButton("user", tr("Account / preferences"), nullptr);
    userButton_->setObjectName(QStringLiteral("topbar-user"));
    userButton_->setPopupMode(QToolButton::InstantPopup);
    auto* userMenu = new QMenu(userButton_);
    userMenu->addAction(tr("Keyboard shortcuts (Ctrl+/)"), [] {
        if (auto* sh = Shell::instance(); sh && sh->commandPalette()) {
            sh->commandPalette()->showPalette(QStringLiteral("?"));
        }
    });
    userMenu->addSeparator();
    userMenu->addAction(tr("Preferences..."), [] {
        if (auto* cmd = Gui::Application::Instance->commandManager().getCommandByName("Std_DlgPreferences")) {
            cmd->invoke(0);
        }
    });
    userMenu->addAction(tr("About LinuxCAD"), [] {
        if (auto* cmd = Gui::Application::Instance->commandManager().getCommandByName("Std_About")) {
            cmd->invoke(0);
        }
    });
    userMenu->addSeparator();
    userMenu->addAction(tr("Quit"), [] {
        if (auto* cmd = Gui::Application::Instance->commandManager().getCommandByName("Std_Quit")) {
            cmd->invoke(0);
        }
    });
    userButton_->setMenu(userMenu);
    row1Layout->addWidget(userButton_);

    outerLayout_->addWidget(row1_);

    // ------------------------------------------------------------------
    // Row 2: ribbon host. The actual Ribbon widget is plugged in later
    // via setRibbonBody(...) by LinuxCadShell::install.
    // ------------------------------------------------------------------
    row2_ = new QWidget(outer_);
    row2_->setObjectName(QStringLiteral("topbar-row2"));
    row2_->setProperty("linuxcadRole", QStringLiteral("topbar-row2"));
    row2_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    row2_->setMinimumHeight(64);
    auto* row2Layout = new QHBoxLayout(row2_);
    row2Layout->setContentsMargins(0, 0, 0, 0);
    row2Layout->setSpacing(0);
    outerLayout_->addWidget(row2_);

    // --- Toolbar's primary widget: a single addWidget() ---------------
    addWidget(outer_);
}

void TopBar::buildLogoMenu()
{
    auto* menu = new QMenu(logoButton_);

    auto runFreeCADCommand = [](const char* name) {
        if (auto* cmd = Gui::Application::Instance->commandManager().getCommandByName(name)) {
            cmd->invoke(0);
        }
    };

    menu->addAction(tr("Re-run setup wizard…"), [] {
        QWidget* mw = Gui::getMainWindow();
        if (Shell::instance() && Shell::instance()->mainWindow()) {
            mw = Shell::instance()->mainWindow();
        }
        FirstRunWizard::runAgain(mw);
    });

    menu->addSeparator();

    menu->addAction(tr("New Sketch... (S)"), [this] {
        if (auto* sh = Shell::instance()) {
            sh->newSketchInteractive();
        }
    });

    menu->addAction(tr("New Project..."), [this] {
        if (auto* pm = Shell::instance() ? Shell::instance()->projectManager() : nullptr) {
            pm->newProjectInteractive(parentWidget());
        }
    }, QKeySequence::New);

    menu->addAction(tr("Open Project..."), [this] {
        if (auto* pm = Shell::instance() ? Shell::instance()->projectManager() : nullptr) {
            pm->openProjectInteractive(parentWidget());
        }
    }, QKeySequence::Open);

    auto* recentMenu = menu->addMenu(tr("Open Recent"));
    if (auto* pm = Shell::instance() ? Shell::instance()->projectManager() : nullptr) {
        const auto recents = pm->recentProjects();
        if (recents.isEmpty()) {
            auto* none = recentMenu->addAction(tr("(no recent projects)"));
            none->setEnabled(false);
        }
        for (const auto& path : recents) {
            recentMenu->addAction(path, [pm, path] { pm->openProject(path); });
        }
    }

    menu->addSeparator();

    menu->addAction(tr("Save"),       [runFreeCADCommand] { runFreeCADCommand("Std_Save"); },
                    QKeySequence::Save);
    menu->addAction(tr("Save As..."), [runFreeCADCommand] { runFreeCADCommand("Std_SaveAs"); },
                    QKeySequence::SaveAs);
    menu->addAction(tr("Save All"),   [runFreeCADCommand] { runFreeCADCommand("Std_SaveAll"); });

    menu->addSeparator();

    menu->addAction(tr("Import..."), [runFreeCADCommand] { runFreeCADCommand("Std_Import"); });
    menu->addAction(tr("Export..."), [runFreeCADCommand] { runFreeCADCommand("Std_Export"); });

    menu->addSeparator();

    menu->addAction(tr("Close Project"), [this] {
        if (auto* pm = Shell::instance() ? Shell::instance()->projectManager() : nullptr) {
            pm->closeProject();
        }
    });

    logoButton_->setMenu(menu);
}

QToolButton* TopBar::makeIconButton(const char* iconName, const QString& tooltip, const char* slot)
{
    auto* btn = new QToolButton(row1_ != nullptr ? row1_ : static_cast<QWidget*>(this));
    btn->setToolTip(tooltip);
    btn->setAutoRaise(true);
    btn->setIcon(BitmapFactory().pixmap(iconName));
    btn->setProperty("linuxcadRole", QStringLiteral("topbar-iconbtn"));
    if (slot) {
        connect(btn, SIGNAL(clicked()), this, slot);
    }
    return btn;
}

void TopBar::setRibbonBody(Ribbon* ribbon)
{
    if (row2_ == nullptr) {
        ribbonHosted_ = ribbon;
        return;
    }
    auto* row2Layout = qobject_cast<QHBoxLayout*>(row2_->layout());
    if (row2Layout == nullptr) {
        ribbonHosted_ = ribbon;
        return;
    }

    // Detach previously-hosted ribbon, if any.
    if (ribbonHosted_ != nullptr && ribbonHosted_ != ribbon) {
        row2Layout->removeWidget(static_cast<QWidget*>(ribbonHosted_));
    }

    ribbonHosted_ = ribbon;

    if (ribbon == nullptr) {
        row2_->setMinimumHeight(64);
        return;
    }

    auto* asWidget = static_cast<QWidget*>(ribbon);
    asWidget->setParent(row2_);
    asWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    row2Layout->addWidget(asWidget);
    asWidget->show();
}

Ribbon* TopBar::ribbonBody() const
{
    return ribbonHosted_;
}

void TopBar::onActiveDocumentChanged()
{
    // Intentional no-op: kept so existing connections from earlier waves
    // continue to compile after the save indicator label was removed.
}

void TopBar::onCommandPaletteRequested()
{
    if (auto* sh = Shell::instance()) {
        if (auto* palette = sh->commandPalette()) {
            palette->showPalette(quickSearch_ ? quickSearch_->text() : QString());
            if (quickSearch_) {
                quickSearch_->clear();
            }
        }
    }
}

void TopBar::onUndo()
{
    if (auto* cmd = Gui::Application::Instance->commandManager().getCommandByName("Std_Undo")) {
        cmd->invoke(0);
    }
}

void TopBar::onRedo()
{
    if (auto* cmd = Gui::Application::Instance->commandManager().getCommandByName("Std_Redo")) {
        cmd->invoke(0);
    }
}

void TopBar::onQuickSearchEdited(const QString& text)
{
    if (text.length() >= 2) {
        if (auto* sh = Shell::instance()) {
            if (auto* palette = sh->commandPalette()) {
                palette->showPalette(text);
            }
        }
    }
}

void TopBar::setRibbonRowInteractive(bool enabled)
{
    if (row2_ != nullptr) {
        row2_->setEnabled(enabled);
    }
}

void TopBar::onAiStateChanged(int state)
{
    if (aiBadge_ == nullptr) {
        return;
    }

    QString stateStr = QStringLiteral("disabled");
    QString label    = tr("AI");
    QString tip      = tr("AI assistant disabled — click to configure");
    switch (state) {
        case 0:
            stateStr = QStringLiteral("disabled");
            label    = tr("AI");
            tip      = tr("AI assistant disabled — click to configure");
            break;
        case 1:
            stateStr = QStringLiteral("idle");
            label    = QStringLiteral("\u2022 AI ready");
            tip      = tr("AI assistant idle — edits will trigger suggestions");
            break;
        case 2:
            stateStr = QStringLiteral("thinking");
            label    = QStringLiteral("\u2022 AI …");
            tip      = tr("AI assistant thinking");
            break;
        case 3:
            stateStr = QStringLiteral("idle");
            label    = QStringLiteral("\u2022 AI ready");
            tip      = tr("Brief cool-off — next edit may trigger suggestions");
            break;
        case 4:
            stateStr = QStringLiteral("error");
            label    = QStringLiteral("\u2022 AI error");
            tip      = tr("AI error — click to reconfigure");
            break;
        default:
            break;
    }

    QSettings qs;
    const bool mockProvider =
        qs.value(QString::fromLatin1(Provider::kSettingProvider), QString())
            .compare(QStringLiteral("mock"), Qt::CaseInsensitive)
        == 0;
    aiBadge_->setProperty(QStringLiteral("linuxcadAiMock"), mockProvider);

    if (mockProvider && (state == 0 || state == 1 || state == 2 || state == 3)) {
        label = QStringLiteral("\u2022 AI mock");
        tip =
            tip + QLatin1Char('\n')
            + tr("(Mock provider — requests stay offline with canned geometry hints.)");
    }

    aiBadge_->setText(label);
    aiBadge_->setProperty("state", stateStr);
    aiBadge_->setToolTip(tip);
    if (aiBadge_->style() != nullptr) {
        aiBadge_->style()->unpolish(aiBadge_);
        aiBadge_->style()->polish(aiBadge_);
    }
}

void TopBar::onAiBadgeClicked()
{
    QWidget* anchor = window();
    if (anchor == nullptr) {
        anchor = parentWidget();
    }
    if (anchor == nullptr) {
        anchor = Gui::getMainWindow();
    }
    if (ConfigureAiDialog::run(anchor) == QDialog::Accepted) {
        if (auto* sh = Gui::LinuxCAD::Shell::instance()) {
            sh->reloadAiProvider();
        }
    }
}

} // namespace LinuxCAD
} // namespace Gui
