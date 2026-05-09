// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QSet>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#endif

#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/WorkbenchManager.h>

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

QString compactTabTitle(const QString& rawTitle)
{
    QString t = rawTitle.simplified();
    t.replace(QStringLiteral("Workbench"), QString(), Qt::CaseInsensitive);
    t.replace(QStringLiteral("Part Design"), QStringLiteral("PD"), Qt::CaseInsensitive);
    t.replace(QStringLiteral("PartDesign"), QStringLiteral("PD"), Qt::CaseInsensitive);
    t = t.simplified();
    if (t.isEmpty()) {
        t = rawTitle.simplified();
    }
    if (t.size() > 28) {
        t = t.left(27) + QStringLiteral("…");
    }
    return t;
}

bool isIndividualViewsToolBar(QToolBar* tb)
{
    if (tb == nullptr) {
        return false;
    }
    static const QSet<QString> kViewDirCommands = {
        QStringLiteral("Std_ViewIsometric"),
        QStringLiteral("Std_ViewFront"),
        QStringLiteral("Std_ViewTop"),
        QStringLiteral("Std_ViewRight"),
        QStringLiteral("Std_ViewRear"),
        QStringLiteral("Std_ViewBottom"),
        QStringLiteral("Std_ViewLeft"),
    };
    QStringList names;
    for (auto* act : tb->actions()) {
        if (act == nullptr || !act->isVisible() || act->isSeparator()) {
            continue;
        }
        const QString n = act->objectName();
        if (n.isEmpty()) {
            continue;
        }
        if (!kViewDirCommands.contains(n)) {
            return false;
        }
        names.append(n);
    }
    // Std workbench exposes exactly these seven; allow subset if user customized.
    return names.size() >= 3;
}

bool shouldSkipByTitle(const QString& title)
{
    const QString t = title.trimmed().toLower();
    if (t.isEmpty()) {
        return true;
    }

    // StdWorkbench: "Individual Views" — redundant with NaviCube + overlay.
    if (t.contains(QStringLiteral("individual")) || t.contains(QStringLiteral("individual view"))) {
        return true;
    }

    // Remove global utility groups that duplicate menubar/logo actions.
    static const QStringList kSkipExact = {
        QStringLiteral("file"),
        QStringLiteral("edit"),
        QStringLiteral("clipboard"),
        QStringLiteral("macro"),
        QStringLiteral("view"),
        QStringLiteral("help"),
        QStringLiteral("workbench"),
        QStringLiteral("structure"),
    };
    if (kSkipExact.contains(t)) {
        return true;
    }

    // Skip noisy Part Design strips (optional clutter); do not use broad
    // "helper" — it matches "Visual Helpers" and hides Sketcher tabs.
    static const QStringList kSkipContains = {
        QStringLiteral("dress-up"),
        QStringLiteral("transformation"),
    };
    for (const QString& token : kSkipContains) {
        if (t.contains(token)) {
            return true;
        }
    }
    return false;
}

/// Toolbars Sketcher shows with Unavailable policy until edit mode; their
/// menu toggles stay hidden even though the toolbar exists and should appear
/// in the ribbon while Sketcher is active.
bool isSketcherEditRibbonToolbar(const QString& titleLower)
{
    static const QStringList kTitles = {
        QStringLiteral("edit mode"),
        QStringLiteral("geometries"),
        QStringLiteral("constraints"),
        QStringLiteral("sketcher tools"),
        QStringLiteral("b-spline tools"),
        QStringLiteral("visual helpers"),
        QStringLiteral("sketcher helpers"),
        QStringLiteral("sketcher edit tools"),
    };
    for (const QString& key : kTitles) {
        if (titleLower == key) {
            return true;
        }
    }
    return false;
}

QString activeWorkbenchName()
{
    if (auto* mgr = Gui::WorkbenchManager::instance()) {
        const std::string active = mgr->activeName();
        if (!active.empty()) {
            return QString::fromUtf8(active.c_str());
        }
    }
    return QStringLiteral("(none)");
}

QList<QAction*> normalizedActionsForRibbon(QToolBar* sourceToolBar)
{
    QList<QAction*> normalized;
    if (sourceToolBar == nullptr) {
        return normalized;
    }

    bool pendingSeparator = false;
    bool hasAnyVisibleCommand = false;
    for (auto* act : sourceToolBar->actions()) {
        if (act == nullptr || !act->isVisible()) {
            continue;
        }
        if (act->isSeparator()) {
            // Keep separators only between visible command clusters.
            pendingSeparator = !normalized.isEmpty();
            continue;
        }
        if (pendingSeparator) {
            normalized.push_back(nullptr);
            pendingSeparator = false;
        }
        normalized.push_back(act);
        hasAnyVisibleCommand = true;
    }

    if (!hasAnyVisibleCommand) {
        normalized.clear();
    }
    return normalized;
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
    : QWidget(parent)
{
    setObjectName(QStringLiteral("LinuxCadRibbon"));
    setProperty("linuxcadRole", QStringLiteral("ribbon"));

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(4);

    tabBar_ = new QTabBar(this);
    tabBar_->setObjectName(QStringLiteral("LinuxCadRibbonTabs"));
    tabBar_->setProperty("linuxcadRole", QStringLiteral("ribbon-tabs"));
    tabBar_->setExpanding(false);
    tabBar_->setDocumentMode(true);
    tabBar_->setMovable(false);
    tabBar_->setUsesScrollButtons(true);
    tabBar_->setDrawBase(false);
    tabBar_->setShape(QTabBar::RoundedNorth);
    tabBar_->setElideMode(Qt::ElideRight);
    tabBar_->setMinimumHeight(28);
    tabBar_->setMaximumHeight(28);
    connect(tabBar_, &QTabBar::currentChanged, this, &Ribbon::onTabChanged);
    outer->addWidget(tabBar_);

    auto* divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("LinuxCadRibbonDivider"));
    divider->setProperty("linuxcadRole", QStringLiteral("ribbon-tab-divider"));
    divider->setFrameShape(QFrame::NoFrame);
    divider->setMinimumHeight(1);
    divider->setMaximumHeight(1);
    outer->addWidget(divider);

    stack_ = new QStackedWidget(this);
    stack_->setObjectName(QStringLiteral("LinuxCadRibbonStack"));
    stack_->setProperty("linuxcadRole", QStringLiteral("ribbon-stack"));
    stack_->setMinimumHeight(72);
    stack_->setMaximumHeight(78);
    connect(tabBar_, &QTabBar::currentChanged, stack_, &QStackedWidget::setCurrentIndex);
    outer->addWidget(stack_);

    rebuildDebounce_ = new QTimer(this);
    rebuildDebounce_->setSingleShot(true);
    rebuildDebounce_->setInterval(140);
    connect(rebuildDebounce_, &QTimer::timeout, this, &Ribbon::onRebuildDebounce);

    // Subscribe to workbench activation. FreeCAD signals via boost::signals2;
    // any throw escaping the slot would tear down the application, so we
    // wrap defensively.
    if (auto* app = Gui::Application::Instance) {
        try {
            app->signalActivateWorkbench.connect([this](const char* /*name*/) {
                scheduleRebuild();
            });
            app->signalInEdit.connect([this](const Gui::ViewProviderDocumentObject&) {
                scheduleRebuild();
            });
            app->signalResetEdit.connect([this](const Gui::ViewProviderDocumentObject&) {
                scheduleRebuild();
            });
        }
        catch (...) {
            // Defensive: never let a signal hookup break the UI.
        }
    }

    // First fill happens once the host MainWindow's toolbars are populated.
    scheduleRebuild();
}

Ribbon::~Ribbon() = default;

void Ribbon::onRebuildDebounce()
{
    rebuild();
    // A second pass catches workbenches (e.g. OpenSCAD) that populate toolbars lazily.
    QTimer::singleShot(260, this, [this]() {
        rebuild();
    });
}

void Ribbon::scheduleRebuild()
{
    if (rebuildDebounce_ != nullptr) {
        rebuildDebounce_->start();
    }
}

bool Ribbon::shouldSkipToolBar(QToolBar* tb) const
{
    if (tb == nullptr) {
        return true;
    }
    if (isLinuxCadChrome(tb->objectName())) {
        return true;
    }
    if (tb->objectName().compare(QLatin1String("Individual Views"), Qt::CaseInsensitive) == 0) {
        return true;
    }
    if (isIndividualViewsToolBar(tb)) {
        return true;
    }
    const QString rawTitle = humanizeToolBarTitle(tb);
    const QString titleLower = rawTitle.trimmed().toLower();
    if (shouldSkipByTitle(rawTitle)) {
        return true;
    }
    const QString wb = activeWorkbenchName();
    const bool sketcherWb = wb.contains(QLatin1String("Sketcher"), Qt::CaseInsensitive);
    const bool allowHiddenToggle = sketcherWb && isSketcherEditRibbonToolbar(titleLower);

    // FreeCAD keeps non-active-workbench toolbars unavailable by hiding their
    // toggle actions. Use that as the active-workbench filter, except for
    // Sketcher edit-mode toolbars (toggle hidden until ForceAvailable).
    if (!allowHiddenToggle) {
        if (auto* viewAction = tb->toggleViewAction()) {
            if (!viewAction->isVisible()) {
                return true;
            }
        }
    }
    return normalizedActionsForRibbon(tb).isEmpty();
}

QWidget* Ribbon::buildPage(QToolBar* sourceToolBar)
{
    const QList<QAction*> normalizedActions = normalizedActionsForRibbon(sourceToolBar);
    if (normalizedActions.isEmpty()) {
        return nullptr;
    }

    auto* page = new QWidget(stack_);
    page->setProperty("linuxcadRole", QStringLiteral("ribbon-page"));

    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(10, 8, 10, 8);
    row->setSpacing(4);
    row->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    for (auto* act : normalizedActions) {
        if (act == nullptr) {
            auto* sep = new QFrame(page);
            sep->setFrameShape(QFrame::VLine);
            sep->setFrameShadow(QFrame::Plain);
            sep->setProperty("linuxcadRole", QStringLiteral("ribbon-inner-sep"));
            row->addWidget(sep);
            continue;
        }
        row->addWidget(makeRibbonButton(act, page));
    }
    row->addStretch();
    return page;
}

void Ribbon::clearTabs()
{
    if (tabBar_ == nullptr || stack_ == nullptr) {
        return;
    }

    QTabBar* const tabs = tabBar_;
    const QSignalBlocker blocker(tabBar_);

    while (tabs->count() > 0) {
        tabs->removeTab(0);
    }
    while (stack_->count() > 0) {
        auto* w = stack_->widget(0);
        stack_->removeWidget(w);
        if (w != nullptr) {
            w->deleteLater();
        }
    }
    tabTitleByIndex_.clear();
}

void Ribbon::onTabChanged(int idx)
{
    if (stack_ != nullptr) {
        stack_->setCurrentIndex(idx);
    }
    if (tabBar_ == nullptr || idx < 0 || idx >= tabBar_->count()) {
        return;
    }

    QSettings s;
    s.beginGroup(QStringLiteral("LinuxCAD/Ribbon/ActiveTab"));
    s.setValue(activeWorkbenchName(), tabTitleByIndex_.value(idx, tabBar_->tabText(idx)));
    s.endGroup();
}

void Ribbon::rebuild()
{
    if (tabBar_ == nullptr || stack_ == nullptr) {
        return;
    }

    auto* mw = Gui::getMainWindow();
    if (mw == nullptr) {
        return;
    }

    const QString prevTitle = tabBar_->currentIndex() >= 0 ? tabBar_->tabText(tabBar_->currentIndex())
                                                            : QString();

    QSettings settings;
    settings.beginGroup(QStringLiteral("LinuxCAD/Ribbon/ActiveTab"));
    QString wantedTitle = settings.value(activeWorkbenchName()).toString();
    settings.endGroup();
    if (wantedTitle.isEmpty()) {
        wantedTitle = prevTitle;
    }

    clearTabs();

    bool addedAny = false;
    const auto toolBars = mw->findChildren<QToolBar*>();
    for (auto* tb : toolBars) {
        if (shouldSkipToolBar(tb)) {
            continue;
        }

        if (tb->isVisible()) {
            tb->setVisible(false);
        }
        if (auto* page = buildPage(tb)) {
            const QString tabTitle = compactTabTitle(humanizeToolBarTitle(tb));
            const int idx = stack_->addWidget(page);
            tabBar_->addTab(tabTitle);
            tabTitleByIndex_.insert(idx, tabTitle);
            addedAny = true;
        }
    }

    if (!addedAny) {
        auto* page = new QWidget(stack_);
        page->setProperty("linuxcadRole", QStringLiteral("ribbon-page"));
        auto* layout = new QHBoxLayout(page);
        layout->setContentsMargins(10, 8, 10, 8);
        layout->setSpacing(0);

        auto* placeholder = new QLabel(tr("No tools for this workbench"), page);
        placeholder->setProperty("linuxcadRole", QStringLiteral("ribbon-empty"));
        placeholder->setAlignment(Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(placeholder);
        layout->addStretch();

        const int idx = stack_->addWidget(page);
        const QString tabTitle = QStringLiteral("—");
        tabBar_->addTab(tabTitle);
        tabTitleByIndex_.insert(idx, tabTitle);
    }

    int targetIndex = 0;
    if (!wantedTitle.isEmpty()) {
        for (int i = 0; i < tabBar_->count(); ++i) {
            if (tabBar_->tabText(i) == wantedTitle) {
                targetIndex = i;
                break;
            }
        }
    }
    tabBar_->setCurrentIndex(targetIndex);
}

} // namespace LinuxCAD
} // namespace Gui

