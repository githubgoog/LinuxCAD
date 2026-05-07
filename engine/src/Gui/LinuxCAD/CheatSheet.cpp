// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QScreen>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#endif

#include "CheatSheet.h"
#include "CommandPalette.h"
#include "Shortcuts.h"

namespace Gui {
namespace LinuxCAD {

namespace {

bool fuzzyMatch(const QString& haystack, const QString& needle)
{
    if (needle.isEmpty()) {
        return true;
    }
    return haystack.contains(needle, Qt::CaseInsensitive);
}

} // namespace

CheatSheet::CheatSheet(CommandPalette* palette, QWidget* parent)
    : QDialog(parent, Qt::Tool | Qt::FramelessWindowHint)
    , palette_(palette)
{
    setObjectName(QStringLiteral("LinuxCadCheatSheet"));
    setProperty("linuxcadRole", QStringLiteral("cheat-sheet"));
    setWindowTitle(tr("Keyboard Shortcuts"));
    setModal(false);
    resize(720, 520);

    buildUi();
    rebuildContent(QString());
}

CheatSheet::~CheatSheet() = default;

void CheatSheet::buildUi()
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(20, 20, 20, 20);
    outer->setSpacing(12);

    auto* header = new QLabel(tr("Keyboard shortcuts"), this);
    {
        QFont f = header->font();
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF() * 1.3);
        header->setFont(f);
    }
    header->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-title"));
    outer->addWidget(header);

    search_ = new QLineEdit(this);
    search_->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-input"));
    search_->setPlaceholderText(tr("Filter shortcuts (Esc to close)"));
    search_->setClearButtonEnabled(true);
    outer->addWidget(search_);

    scroll_ = new QScrollArea(this);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setWidgetResizable(true);
    scroll_->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-scroll"));
    outer->addWidget(scroll_, 1);

    content_ = new QWidget(scroll_);
    contentLayout_ = new QVBoxLayout(content_);
    contentLayout_->setContentsMargins(0, 0, 0, 0);
    contentLayout_->setSpacing(4);
    scroll_->setWidget(content_);

    auto* footer = new QLabel(tr("Press the key shown to invoke the command. Esc closes this dialog."),
                              this);
    footer->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-footer"));
    outer->addWidget(footer);

    connect(search_, &QLineEdit::textChanged, this, &CheatSheet::onFilterChanged);
}

void CheatSheet::rebuildContent(const QString& filter)
{
    if (contentLayout_ == nullptr) {
        return;
    }

    // Clear previous content.
    while (contentLayout_->count() > 0) {
        QLayoutItem* item = contentLayout_->takeAt(0);
        if (item == nullptr) {
            continue;
        }
        if (auto* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    // Bucket entries by category in input order.
    QMap<QString, QList<ShortcutEntry>> grouped;
    QStringList categoryOrder;
    for (const auto& e : Shortcuts::curated()) {
        const QString hay = QString::fromUtf8(e.friendlyName)
                            + QStringLiteral("  ")
                            + QString::fromLatin1(e.keySequence)
                            + QStringLiteral("  ")
                            + QString::fromUtf8(e.commandName);
        if (!fuzzyMatch(hay, filter)) {
            continue;
        }
        const QString cat = QString::fromUtf8(e.category);
        if (!grouped.contains(cat)) {
            categoryOrder << cat;
        }
        grouped[cat].append(e);
    }

    if (grouped.isEmpty()) {
        auto* none = new QLabel(tr("No shortcuts match \"%1\".").arg(filter), content_);
        none->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-empty"));
        contentLayout_->addWidget(none);
        contentLayout_->addStretch();
        return;
    }

    for (const auto& cat : categoryOrder) {
        auto* section = new QLabel(cat.toUpper(), content_);
        section->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-section"));
        contentLayout_->addWidget(section);

        for (const auto& e : grouped[cat]) {
            auto* row = new QWidget(content_);
            row->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-row"));
            auto* h = new QHBoxLayout(row);
            h->setContentsMargins(4, 4, 4, 4);
            h->setSpacing(12);

            auto* nameLabel = new QLabel(QString::fromUtf8(e.friendlyName), row);
            nameLabel->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-name"));
            h->addWidget(nameLabel, 1);

            auto* keyLabel = new QLabel(QString::fromLatin1(e.keySequence), row);
            keyLabel->setProperty("linuxcadRole", QStringLiteral("cheat-sheet-key"));
            keyLabel->setAlignment(Qt::AlignCenter);
            h->addWidget(keyLabel, 0);

            contentLayout_->addWidget(row);
        }
    }

    contentLayout_->addStretch();
}

void CheatSheet::onFilterChanged(const QString& text)
{
    rebuildContent(text);
}

void CheatSheet::keyPressEvent(QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QDialog::keyPressEvent(ev);
}

void CheatSheet::showOverlay()
{
    if (search_ != nullptr) {
        search_->clear();
        search_->setFocus();
    }
    rebuildContent(QString());
    if (auto* screen = QApplication::primaryScreen()) {
        const QRect g = screen->availableGeometry();
        move(g.center() - QPoint(width() / 2, height() / 2));
    }
    show();
    raise();
    activateWindow();
}

} // namespace LinuxCAD
} // namespace Gui
