// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QShowEvent>
#include <QVBoxLayout>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>

#include "CommandPalette.h"

namespace Gui {
namespace LinuxCAD {

namespace {

QString stripAccelerators(const char* s)
{
    if (!s) {
        return QString();
    }
    return QString::fromUtf8(s).remove(QLatin1Char('&'));
}

bool fuzzyMatch(const QString& haystack, const QString& needle)
{
    if (needle.isEmpty()) {
        return true;
    }
    int hi = 0;
    int ni = 0;
    while (hi < haystack.size() && ni < needle.size()) {
        if (haystack[hi] == needle[ni]) {
            ++ni;
        }
        ++hi;
    }
    return ni == needle.size();
}

} // namespace

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("LinuxCadCommandPalette"));
    setProperty("linuxcadRole", QStringLiteral("command-palette"));
    setModal(true);
    resize(560, 420);

    buildUi();
}

CommandPalette::~CommandPalette() = default;

void CommandPalette::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    query_ = new QLineEdit(this);
    query_->setPlaceholderText(tr("Type a command..."));
    query_->setProperty("linuxcadRole", QStringLiteral("command-palette-input"));
    layout->addWidget(query_);
    connect(query_, &QLineEdit::textChanged, this, &CommandPalette::onQueryChanged);

    list_ = new QListWidget(this);
    list_->setProperty("linuxcadRole", QStringLiteral("command-palette-list"));
    list_->setUniformItemSizes(true);
    layout->addWidget(list_, 1);
    connect(list_, &QListWidget::itemActivated, this, &CommandPalette::onItemActivated);

    footer_ = new QLabel(tr("Enter to run · Esc to close"), this);
    footer_->setProperty("linuxcadRole", QStringLiteral("command-palette-footer"));
    layout->addWidget(footer_);
}

void CommandPalette::rebuildIndex()
{
    all_.clear();
    auto* app = Gui::Application::Instance;
    if (!app) {
        return;
    }
    const auto cmds = app->commandManager().getAllCommands();
    all_.reserve(cmds.size());
    for (auto* cmd : cmds) {
        if (!cmd) {
            continue;
        }
        Entry e;
        e.cmd      = cmd;
        e.name     = QString::fromUtf8(cmd->getName() ? cmd->getName() : "");
        e.menuText = stripAccelerators(cmd->getMenuText());
        e.tooltip  = stripAccelerators(cmd->getToolTipText());
        e.haystack = (e.menuText + QLatin1Char(' ') + e.tooltip + QLatin1Char(' ') + e.name).toLower();
        all_.append(e);
    }
}

void CommandPalette::showPalette(const QString& initialQuery)
{
    rebuildIndex();
    query_->setText(initialQuery);
    onQueryChanged(initialQuery);
    if (auto* p = parentWidget()) {
        const QPoint center = p->mapToGlobal(p->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
    show();
    raise();
    query_->setFocus();
    if (!initialQuery.isEmpty()) {
        query_->selectAll();
    }
}

void CommandPalette::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    if (list_ && list_->count() > 0 && !list_->currentItem()) {
        list_->setCurrentRow(0);
    }
}

void CommandPalette::onQueryChanged(const QString& q)
{
    list_->clear();
    const QString needle = q.trimmed().toLower();
    int added = 0;
    for (const auto& e : all_) {
        if (e.menuText.isEmpty() && e.name.isEmpty()) {
            continue;
        }
        // Fast path: substring match has priority.
        const bool subMatch = !needle.isEmpty()
            && (e.menuText.contains(needle, Qt::CaseInsensitive)
                || e.name.contains(needle, Qt::CaseInsensitive)
                || e.tooltip.contains(needle, Qt::CaseInsensitive));
        const bool fuzzy = needle.isEmpty() || subMatch || fuzzyMatch(e.haystack, needle);
        if (!fuzzy) {
            continue;
        }
        auto* item = new QListWidgetItem(list_);
        const QString title = e.menuText.isEmpty() ? e.name : e.menuText;
        item->setText(title);
        if (!e.tooltip.isEmpty() && e.tooltip != title) {
            item->setToolTip(e.tooltip);
        }
        if (e.cmd) {
            const char* px = e.cmd->getPixmap();
            if (px && *px) {
                item->setIcon(BitmapFactory().pixmap(px));
            }
        }
        item->setData(Qt::UserRole, QVariant::fromValue<void*>(e.cmd));
        ++added;
        if (added >= 200) {
            // Cap render cost; the user can keep typing to narrow further.
            break;
        }
    }
    if (list_->count() > 0) {
        list_->setCurrentRow(0);
    }
}

void CommandPalette::onItemActivated(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    auto* cmd = static_cast<Gui::Command*>(item->data(Qt::UserRole).value<void*>());
    if (!cmd) {
        return;
    }
    executeSelected(cmd);
}

void CommandPalette::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Escape:
            close();
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (auto* item = list_->currentItem()) {
                onItemActivated(item);
            }
            return;
        case Qt::Key_Up:
            if (list_->count() > 0) {
                list_->setCurrentRow(qMax(0, list_->currentRow() - 1));
            }
            return;
        case Qt::Key_Down:
            if (list_->count() > 0) {
                list_->setCurrentRow(qMin(list_->count() - 1, list_->currentRow() + 1));
            }
            return;
        default:
            break;
    }
    QDialog::keyPressEvent(event);
}

void CommandPalette::executeSelected(Gui::Command* cmd)
{
    if (!cmd) {
        return;
    }
    close();
    try {
        cmd->invoke(0);
    }
    catch (const std::exception& e) {
        Base::Console().error("LinuxCAD: command '%s' threw: %s\n",
                              cmd->getName() ? cmd->getName() : "(unknown)", e.what());
    }
    catch (...) {
        Base::Console().error("LinuxCAD: command '%s' threw unknown exception\n",
                              cmd->getName() ? cmd->getName() : "(unknown)");
    }
}

} // namespace LinuxCAD
} // namespace Gui
