// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSizePolicy>
#include <QSvgRenderer>
#include <QToolButton>
#include <QVBoxLayout>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>

#include <algorithm>
#include <string>

#include "LinuxCadStart.h"

namespace {
QPixmap loadWordmarkPixmap()
{
    constexpr int targetHeight = 52;
    const QString path = QStringLiteral(":/linuxcad/branding/wordmark.svg");
    QSvgRenderer renderer(path);
    if (renderer.isValid()) {
        const QSize def = renderer.defaultSize();
        if (def.height() > 0 && def.width() > 0) {
            const qreal scale = static_cast<qreal>(targetHeight) / static_cast<qreal>(def.height());
            const int width = qMax(1, static_cast<int>((def.width() * scale) + 0.5));
            QPixmap pm(width, targetHeight);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            renderer.render(&p, QRectF(0, 0, width, static_cast<qreal>(targetHeight)));
            return pm;
        }
    }
    return {};
}

void invokeFreeCadCommand(const char* commandName)
{
    if (Gui::Application::Instance == nullptr || commandName == nullptr || *commandName == '\0') {
        return;
    }
    if (auto* cmd = Gui::Application::Instance->commandManager().getCommandByName(commandName)) {
        cmd->invoke(0);
    }
}
} // namespace

namespace Gui {
namespace LinuxCAD {

LinuxCadStart::LinuxCadStart(Gui::MainWindow* mainWindow, QWidget* parent)
    : QWidget(parent)
    , mainWindow_(mainWindow)
{
    setObjectName(QStringLiteral("LinuxCadStart"));
    setProperty("linuxcadRole", QStringLiteral("start-root"));
    setAttribute(Qt::WA_StyledBackground, true);

    buildUi();
    refreshRecents();
}

void LinuxCadStart::setHostWidget(QWidget* hostWidget)
{
    if (hostWidget_ == hostWidget) {
        syncToHost();
        return;
    }

    if (hostWidget_ != nullptr) {
        hostWidget_->removeEventFilter(this);
    }

    hostWidget_ = hostWidget;
    if (hostWidget_ != nullptr) {
        hostWidget_->installEventFilter(this);
    }
    syncToHost();
}

void LinuxCadStart::syncToHost()
{
    if (hostWidget_ == nullptr || parentWidget() == nullptr) {
        return;
    }
    const QPoint origin = hostWidget_->mapTo(parentWidget(), QPoint(0, 0));
    setGeometry(QRect(origin, hostWidget_->size()));
}

void LinuxCadStart::refreshRecents()
{
    clearRecentEntries();

    const QStringList recents = loadRecentPaths(8);
    if (recents.isEmpty()) {
        if (recentEmptyState_ != nullptr) {
            recentEmptyState_->setVisible(true);
        }
        return;
    }

    if (recentEmptyState_ != nullptr) {
        recentEmptyState_->setVisible(false);
    }
    for (const QString& recent : recents) {
        addRecentEntry(recent);
    }
}

bool LinuxCadStart::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == hostWidget_ && event != nullptr) {
        switch (event->type()) {
            case QEvent::Resize:
            case QEvent::Move:
            case QEvent::Show:
                syncToHost();
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void LinuxCadStart::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 24, 32, 20);
    root->setSpacing(16);

    auto* hero = new QWidget(this);
    hero->setProperty("linuxcadRole", QStringLiteral("start-hero"));
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(0, 0, 0, 0);
    heroLayout->setSpacing(10);

    logoLabel_ = new QLabel(hero);
    logoLabel_->setProperty("linuxcadRole", QStringLiteral("start-wordmark"));
    logoLabel_->setPixmap(loadWordmarkPixmap());
    logoLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    heroLayout->addWidget(logoLabel_, 0, Qt::AlignLeft);

    auto* tagline = new QLabel(tr("Precision CAD for Linux-first engineering workflows."), hero);
    tagline->setWordWrap(true);
    tagline->setProperty("linuxcadRole", QStringLiteral("start-tagline"));
    heroLayout->addWidget(tagline);

    auto* ctaRow = new QHBoxLayout();
    ctaRow->setContentsMargins(0, 0, 0, 0);
    ctaRow->setSpacing(10);

    auto* newButton = new QPushButton(tr("New"), hero);
    newButton->setProperty("linuxcadRole", QStringLiteral("start-cta-new"));
    newButton->setMinimumWidth(120);
    connect(newButton, &QPushButton::clicked, this, [] { invokeFreeCadCommand("Std_New"); });
    ctaRow->addWidget(newButton);

    auto* openButton = new QPushButton(tr("Open"), hero);
    openButton->setProperty("linuxcadRole", QStringLiteral("start-cta-open"));
    openButton->setMinimumWidth(120);
    connect(openButton, &QPushButton::clicked, this, [] { invokeFreeCadCommand("Std_Open"); });
    ctaRow->addWidget(openButton);
    ctaRow->addStretch(1);
    heroLayout->addLayout(ctaRow);

    root->addWidget(hero);

    auto* recentsFrame = new QFrame(this);
    recentsFrame->setFrameShape(QFrame::StyledPanel);
    recentsFrame->setProperty("linuxcadRole", QStringLiteral("start-recents"));
    auto* recentsLayout = new QVBoxLayout(recentsFrame);
    recentsLayout->setContentsMargins(14, 12, 14, 12);
    recentsLayout->setSpacing(8);

    auto* recentsTitle = new QLabel(tr("Recent files"), recentsFrame);
    recentsTitle->setProperty("linuxcadRole", QStringLiteral("start-recents-title"));
    recentsLayout->addWidget(recentsTitle);

    auto* recentListHost = new QWidget(recentsFrame);
    recentListLayout_ = new QVBoxLayout(recentListHost);
    recentListLayout_->setContentsMargins(0, 0, 0, 0);
    recentListLayout_->setSpacing(6);
    recentsLayout->addWidget(recentListHost);

    recentEmptyState_ = new QLabel(tr("No recent files yet. Start by creating a new design."), recentsFrame);
    recentEmptyState_->setProperty("linuxcadRole", QStringLiteral("start-recents-empty"));
    recentEmptyState_->setVisible(false);
    recentsLayout->addWidget(recentEmptyState_);

    root->addWidget(recentsFrame);
    root->addStretch(1);

    auto* footer = new QLabel(tr("LinuxCAD is licensed under the GNU LGPL v2.1 or later."), this);
    footer->setWordWrap(true);
    footer->setProperty("linuxcadRole", QStringLiteral("start-footer"));
    root->addWidget(footer);
}

void LinuxCadStart::clearRecentEntries()
{
    if (recentListLayout_ == nullptr) {
        return;
    }
    while (QLayoutItem* item = recentListLayout_->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void LinuxCadStart::addRecentEntry(const QString& filePath)
{
    if (recentListLayout_ == nullptr) {
        return;
    }

    auto* button = new QToolButton(this);
    button->setProperty("linuxcadRole", QStringLiteral("start-recent-item"));
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setText(filePath);
    button->setAutoRaise(false);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    button->setToolTip(filePath);
    button->setMinimumHeight(30);
    connect(button, &QToolButton::clicked, this, [this, filePath] { openRecentPath(filePath); });

    recentListLayout_->addWidget(button);
}

void LinuxCadStart::openRecentPath(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }
    try {
        App::GetApplication().openDocument(filePath.toUtf8().constData());
    }
    catch (...) {
        // Ignore failures; document open flow already reports user-facing errors.
    }
}

QStringList LinuxCadStart::loadRecentPaths(int maxEntries)
{
    QStringList recents;
    try {
        auto group = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/RecentFiles");
        const int count = group->GetInt("RecentFiles", 0);
        const int cap = std::max(0, std::min(count, maxEntries));
        for (int i = 0; i < cap; ++i) {
            const std::string key = "MRU" + std::to_string(i);
            const std::string val = group->GetASCII(key.c_str(), "");
            if (!val.empty()) {
                recents.append(QString::fromStdString(val));
            }
        }
    }
    catch (...) {
        // Defensive: if parameter storage is unavailable keep recents empty.
    }
    return recents;
}

} // namespace LinuxCAD
} // namespace Gui
