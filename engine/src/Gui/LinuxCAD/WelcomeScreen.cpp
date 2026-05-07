// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>
#include <QWidget>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>

#include "ProjectManager.h"
#include "WelcomeScreen.h"

namespace Gui {
namespace LinuxCAD {

WelcomeScreen::WelcomeScreen(ProjectManager* manager, QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::WindowStaysOnTopHint)
    , manager_(manager)
{
    setObjectName(QStringLiteral("LinuxCadWelcomeScreen"));
    setWindowTitle(tr("Welcome to LinuxCAD"));
    setProperty("linuxcadRole", QStringLiteral("welcome"));
    setMinimumSize(720, 460);

    buildUi();

    if (manager_) {
        connect(manager_, &ProjectManager::recentProjectsChanged,
                this, &WelcomeScreen::refresh);
        connect(manager_, &ProjectManager::projectChanged,
                this, &WelcomeScreen::refresh);
    }
    refresh();
}

WelcomeScreen::~WelcomeScreen() = default;

void WelcomeScreen::buildUi()
{
    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // --- Left: hero / actions -------------------------------------------
    auto* left = new QWidget(this);
    left->setProperty("linuxcadRole", QStringLiteral("welcome-left"));
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(28, 28, 28, 28);
    leftLayout->setSpacing(12);

    heroLabel_ = new QLabel(tr("LinuxCAD"), left);
    heroLabel_->setProperty("linuxcadRole", QStringLiteral("welcome-hero"));
    {
        QFont f = heroLabel_->font();
        f.setPointSizeF(f.pointSizeF() * 2.2);
        f.setBold(true);
        heroLabel_->setFont(f);
    }
    leftLayout->addWidget(heroLabel_);

    auto* tagline = new QLabel(tr("Powered by the FreeCAD engine. Friendly, fast, yours."), left);
    tagline->setProperty("linuxcadRole", QStringLiteral("welcome-tagline"));
    leftLayout->addWidget(tagline);

    leftLayout->addSpacing(20);

    newBtn_ = new QPushButton(tr("New Project"), left);
    newBtn_->setProperty("linuxcadRole", QStringLiteral("welcome-cta-primary"));
    leftLayout->addWidget(newBtn_);

    openBtn_ = new QPushButton(tr("Open Project..."), left);
    openBtn_->setProperty("linuxcadRole", QStringLiteral("welcome-cta"));
    leftLayout->addWidget(openBtn_);

    startBtn_ = new QPushButton(tr("Open Without Project"), left);
    startBtn_->setProperty("linuxcadRole", QStringLiteral("welcome-cta-quiet"));
    startBtn_->setToolTip(tr("Use FreeCAD's classic Start workbench"));
    leftLayout->addWidget(startBtn_);

    leftLayout->addStretch();

    auto* footer = new QLabel(tr("LinuxCAD is built on top of FreeCAD (LGPL-2.1+)."), left);
    footer->setProperty("linuxcadRole", QStringLiteral("welcome-footer"));
    footer->setWordWrap(true);
    leftLayout->addWidget(footer);

    outer->addWidget(left, 0);

    // --- Right: recent projects -----------------------------------------
    auto* right = new QWidget(this);
    right->setProperty("linuxcadRole", QStringLiteral("welcome-right"));
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(28, 28, 28, 28);
    rightLayout->setSpacing(12);

    auto* recentTitle = new QLabel(tr("Recent Projects"), right);
    recentTitle->setProperty("linuxcadRole", QStringLiteral("welcome-section-title"));
    {
        QFont f = recentTitle->font();
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF() * 1.1);
        recentTitle->setFont(f);
    }
    rightLayout->addWidget(recentTitle);

    recentList_ = new QListWidget(right);
    recentList_->setProperty("linuxcadRole", QStringLiteral("welcome-recent"));
    recentList_->setAlternatingRowColors(true);
    rightLayout->addWidget(recentList_, 1);

    outer->addWidget(right, 1);

    // --- Connections -----------------------------------------------------
    connect(newBtn_,   &QPushButton::clicked, this, &WelcomeScreen::onNewProject);
    connect(openBtn_,  &QPushButton::clicked, this, &WelcomeScreen::onOpenProject);
    connect(startBtn_, &QPushButton::clicked, this, &WelcomeScreen::onOpenStartWorkbench);
    connect(recentList_, &QListWidget::itemDoubleClicked,
            this, &WelcomeScreen::onRecentDoubleClicked);
}

void WelcomeScreen::refresh()
{
    if (!recentList_) {
        return;
    }
    recentList_->clear();
    if (!manager_) {
        return;
    }
    for (const auto& path : manager_->recentProjects()) {
        const QFileInfo fi(path);
        auto* item = new QListWidgetItem(recentList_);
        item->setText(fi.completeBaseName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
    }
    if (recentList_->count() == 0) {
        auto* hint = new QListWidgetItem(recentList_);
        hint->setText(tr("No recent projects yet — create your first one."));
        hint->setFlags(hint->flags() & ~Qt::ItemIsSelectable);
        QFont f = hint->font();
        f.setItalic(true);
        hint->setFont(f);
    }
}

void WelcomeScreen::showCentered()
{
    if (auto* screen = QApplication::primaryScreen()) {
        const QRect g = screen->availableGeometry();
        const QSize sz = size().expandedTo(QSize(720, 460));
        move(g.center() - QPoint(sz.width() / 2, sz.height() / 2));
    }
    show();
    raise();
    activateWindow();
}

void WelcomeScreen::onNewProject()
{
    if (manager_) {
        manager_->newProjectInteractive(this);
        if (manager_->hasActiveProject()) {
            close();
        }
    }
}

void WelcomeScreen::onOpenProject()
{
    if (manager_) {
        manager_->openProjectInteractive(this);
        if (manager_->hasActiveProject()) {
            close();
        }
    }
}

void WelcomeScreen::onRecentDoubleClicked(QListWidgetItem* item)
{
    if (!item || !manager_) {
        return;
    }
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) {
        return;
    }
    if (manager_->openProject(path)) {
        close();
    }
}

void WelcomeScreen::onOpenStartWorkbench()
{
    if (auto* app = Gui::Application::Instance) {
        app->activateWorkbench("StartWorkbench");
    }
    close();
}

} // namespace LinuxCAD
} // namespace Gui
