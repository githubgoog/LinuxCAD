// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QWidgetAction>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>

#include "TopBar.h"
#include "CommandPalette.h"
#include "LinuxCadShell.h"
#include "ProjectManager.h"

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

    // React to workbench changes so our switcher stays in sync with FreeCAD.
    if (auto* app = Gui::Application::Instance) {
        // Re-fill on construction; FreeCAD signals don't directly tell us when
        // the workbench *list* changes, so we expose refreshWorkbenchList()
        // for callers that know.
        refreshWorkbenchList();

        try {
            app->signalActivateWorkbench.connect([this](const char* name) {
                onWorkbenchActivated(QString::fromUtf8(name ? name : ""));
            });
        }
        catch (...) {
            // Defensive: never let a signal hookup break the UI.
        }
    }

    // Global Ctrl+K shortcut for the command palette.
    auto* sc = new QShortcut(QKeySequence(QStringLiteral("Ctrl+K")), this);
    connect(sc, &QShortcut::activated, this, &TopBar::onCommandPaletteRequested);
}

TopBar::~TopBar() = default;

void TopBar::buildLayout()
{
    // --- Project button (with dropdown menu) ----------------------------
    projectButton_ = new QToolButton(this);
    projectButton_->setText(tr("Project"));
    projectButton_->setToolTip(tr("Project actions"));
    projectButton_->setPopupMode(QToolButton::InstantPopup);
    projectButton_->setProperty("linuxcadRole", QStringLiteral("topbar-project"));
    buildProjectMenu();
    addWidget(projectButton_);

    addSeparator();

    // --- Workbench switcher ---------------------------------------------
    auto* wbLabel = new QLabel(tr("Workbench"), this);
    wbLabel->setProperty("linuxcadRole", QStringLiteral("topbar-label"));
    wbLabel->setContentsMargins(8, 0, 4, 0);
    addWidget(wbLabel);

    workbenchSwitcher_ = new QComboBox(this);
    workbenchSwitcher_->setMinimumWidth(220);
    workbenchSwitcher_->setProperty("linuxcadRole", QStringLiteral("topbar-workbench"));
    addWidget(workbenchSwitcher_);
    connect(workbenchSwitcher_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TopBar::onWorkbenchSelectionChanged);

    addSeparator();

    // --- Quick search / command palette ---------------------------------
    quickSearch_ = new QLineEdit(this);
    quickSearch_->setPlaceholderText(tr("Search commands  (Ctrl+K)"));
    quickSearch_->setMinimumWidth(280);
    quickSearch_->setClearButtonEnabled(true);
    quickSearch_->setProperty("linuxcadRole", QStringLiteral("topbar-search"));
    addWidget(quickSearch_);
    connect(quickSearch_, &QLineEdit::textEdited, this, &TopBar::onQuickSearchEdited);
    connect(quickSearch_, &QLineEdit::returnPressed, this, &TopBar::onCommandPaletteRequested);

    paletteButton_ = makeIconButton("command", tr("Open command palette (Ctrl+K)"),
                                    SLOT(onCommandPaletteRequested()));
    addWidget(paletteButton_);

    // Spacer pushes right-side controls to the end.
    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    addWidget(spacer);

    // --- Undo / Redo ----------------------------------------------------
    undoButton_ = makeIconButton("edit-undo", tr("Undo (Ctrl+Z)"), SLOT(onUndo()));
    redoButton_ = makeIconButton("edit-redo", tr("Redo (Ctrl+Shift+Z)"), SLOT(onRedo()));
    addWidget(undoButton_);
    addWidget(redoButton_);

    addSeparator();

    // --- Save indicator -------------------------------------------------
    saveIndicator_ = new QLabel(tr("Saved"), this);
    saveIndicator_->setProperty("linuxcadRole", QStringLiteral("topbar-save"));
    saveIndicator_->setContentsMargins(8, 0, 8, 0);
    addWidget(saveIndicator_);

    // --- AI badge -------------------------------------------------------
    aiBadge_ = new QToolButton(this);
    aiBadge_->setText(tr("AI"));
    aiBadge_->setProperty("linuxcadRole", QStringLiteral("ai-status"));
    aiBadge_->setProperty("state", QStringLiteral("disabled"));
    aiBadge_->setToolTip(tr("AI assistant - click to configure"));
    aiBadge_->setAutoRaise(false);
    addWidget(aiBadge_);
    connect(aiBadge_, &QToolButton::clicked, this, &TopBar::onAiBadgeClicked);

    // --- User area ------------------------------------------------------
    userButton_ = makeIconButton("user", tr("Account / preferences"), nullptr);
    userButton_->setPopupMode(QToolButton::InstantPopup);
    auto* userMenu = new QMenu(userButton_);
    userMenu->addAction(tr("Keyboard shortcuts (Ctrl+/)"), [] {
        if (auto* sh = Shell::instance()) {
            sh->showCheatSheet();
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
    addWidget(userButton_);
}

void TopBar::buildProjectMenu()
{
    auto* menu = new QMenu(projectButton_);

    auto runFreeCADCommand = [](const char* name) {
        if (auto* cmd = Gui::Application::Instance->commandManager().getCommandByName(name)) {
            cmd->invoke(0);
        }
    };

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

    projectButton_->setMenu(menu);
}

QToolButton* TopBar::makeIconButton(const char* iconName, const QString& tooltip, const char* slot)
{
    auto* btn = new QToolButton(this);
    btn->setToolTip(tooltip);
    btn->setAutoRaise(true);
    btn->setIcon(BitmapFactory().pixmap(iconName));
    btn->setProperty("linuxcadRole", QStringLiteral("topbar-iconbtn"));
    if (slot) {
        connect(btn, SIGNAL(clicked()), this, slot);
    }
    return btn;
}

void TopBar::refreshWorkbenchList()
{
    if (!workbenchSwitcher_) {
        return;
    }
    auto* app = Gui::Application::Instance;
    if (!app) {
        return;
    }

    QSignalBlocker block(workbenchSwitcher_);
    workbenchSwitcher_->clear();

    QStringList wbs = app->workbenches();
    wbs.sort(Qt::CaseInsensitive);
    for (const auto& wb : wbs) {
        const QString display = app->workbenchMenuText(wb);
        const QPixmap icon = app->workbenchIcon(wb);
        if (icon.isNull()) {
            workbenchSwitcher_->addItem(display.isEmpty() ? wb : display, wb);
        }
        else {
            workbenchSwitcher_->addItem(QIcon(icon), display.isEmpty() ? wb : display, wb);
        }
    }
}

void TopBar::onWorkbenchActivated(const QString& name)
{
    if (!workbenchSwitcher_ || updatingSwitcher_) {
        return;
    }
    QSignalBlocker block(workbenchSwitcher_);
    for (int i = 0; i < workbenchSwitcher_->count(); ++i) {
        if (workbenchSwitcher_->itemData(i).toString() == name) {
            workbenchSwitcher_->setCurrentIndex(i);
            return;
        }
    }
}

void TopBar::onActiveDocumentChanged()
{
    if (!saveIndicator_) {
        return;
    }
    auto* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        saveIndicator_->setText(tr("No document"));
        saveIndicator_->setProperty("state", QStringLiteral("none"));
    }
    else if (doc->isTouched()) {
        saveIndicator_->setText(tr("Unsaved changes"));
        saveIndicator_->setProperty("state", QStringLiteral("dirty"));
    }
    else {
        saveIndicator_->setText(tr("Saved"));
        saveIndicator_->setProperty("state", QStringLiteral("saved"));
    }
    saveIndicator_->style()->unpolish(saveIndicator_);
    saveIndicator_->style()->polish(saveIndicator_);
}

void TopBar::onProjectButtonClicked()
{
    // Menu is set as InstantPopup; Qt handles the click. No-op slot retained
    // in case we wire programmatic open later.
}

void TopBar::onWorkbenchSelectionChanged(int index)
{
    if (!workbenchSwitcher_ || index < 0) {
        return;
    }
    const QString wbName = workbenchSwitcher_->itemData(index).toString();
    if (wbName.isEmpty()) {
        return;
    }
    updatingSwitcher_ = true;
    Gui::Application::Instance->activateWorkbench(wbName.toUtf8().constData());
    updatingSwitcher_ = false;
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

void TopBar::onAiStateChanged(int state)
{
    if (aiBadge_ == nullptr) {
        return;
    }
    QString stateStr = QStringLiteral("disabled");
    QString label    = tr("AI");
    QString tip      = tr("AI assistant disabled - click to configure");
    switch (state) {
        case 0: stateStr = QStringLiteral("disabled"); label = tr("AI");          tip = tr("AI assistant disabled"); break;
        case 1: stateStr = QStringLiteral("idle");     label = tr("AI · Ready");  tip = tr("AI assistant idle - changes will trigger suggestions"); break;
        case 2: stateStr = QStringLiteral("thinking"); label = tr("AI · ...");    tip = tr("AI assistant thinking"); break;
        case 3: stateStr = QStringLiteral("idle");     label = tr("AI · Ready");  tip = tr("AI cooled off, will resume on next change"); break;
        case 4: stateStr = QStringLiteral("error");    label = tr("AI · Error");  tip = tr("AI assistant ran into an error - click for details"); break;
        default: break;
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
    // Open a small inline AI settings dialog. Reuses QSettings-backed values
    // and persists immediately so the next request picks them up.
    QDialog dlg(this);
    dlg.setWindowTitle(tr("AI assistant"));
    dlg.setModal(true);
    auto* layout = new QFormLayout(&dlg);

    QSettings s;
    auto* enabledCb = new QCheckBox(tr("Enable AI suggestions"), &dlg);
    enabledCb->setChecked(s.value(QStringLiteral("LinuxCAD/AI/Enabled"), false).toBool());

    auto* providerCb = new QComboBox(&dlg);
    providerCb->addItem(tr("OpenAI / OpenAI-compatible"), QStringLiteral("openai-compatible"));
    providerCb->addItem(tr("Anthropic Claude"),           QStringLiteral("anthropic"));
    const QString persistedProvider =
        s.value(QStringLiteral("LinuxCAD/AI/Provider"), QStringLiteral("openai-compatible")).toString();
    for (int i = 0; i < providerCb->count(); ++i) {
        if (providerCb->itemData(i).toString() == persistedProvider) {
            providerCb->setCurrentIndex(i);
        }
    }

    auto* endpointEdit = new QLineEdit(
        s.value(QStringLiteral("LinuxCAD/AI/Endpoint"),
                QStringLiteral("https://api.openai.com/v1")).toString(),
        &dlg);

    auto* modelEdit = new QLineEdit(
        s.value(QStringLiteral("LinuxCAD/AI/Model"), QStringLiteral("gpt-4o-mini")).toString(),
        &dlg);

    auto* keyEdit = new QLineEdit(s.value(QStringLiteral("LinuxCAD/AI/ApiKey")).toString(), &dlg);
    keyEdit->setEchoMode(QLineEdit::Password);
    keyEdit->setPlaceholderText(tr("Bring your own API key"));

    auto* consentCb = new QCheckBox(
        tr("I understand modeling context will be sent to the chosen provider"),
        &dlg);
    consentCb->setChecked(s.value(QStringLiteral("LinuxCAD/AI/ConsentGivenV1"), false).toBool());

    layout->addRow(enabledCb);
    layout->addRow(tr("Provider:"),  providerCb);
    layout->addRow(tr("Endpoint:"),  endpointEdit);
    layout->addRow(tr("Model:"),     modelEdit);
    layout->addRow(tr("API key:"),   keyEdit);
    layout->addRow(consentCb);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        s.setValue(QStringLiteral("LinuxCAD/AI/Enabled"),       enabledCb->isChecked());
        s.setValue(QStringLiteral("LinuxCAD/AI/Provider"),      providerCb->currentData().toString());
        s.setValue(QStringLiteral("LinuxCAD/AI/Endpoint"),      endpointEdit->text().trimmed());
        s.setValue(QStringLiteral("LinuxCAD/AI/Model"),         modelEdit->text().trimmed());
        s.setValue(QStringLiteral("LinuxCAD/AI/ApiKey"),        keyEdit->text());
        s.setValue(QStringLiteral("LinuxCAD/AI/ConsentGivenV1"),consentCb->isChecked());
    }
}

} // namespace LinuxCAD
} // namespace Gui
