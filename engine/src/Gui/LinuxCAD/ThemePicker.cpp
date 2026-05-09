// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSettings>
#include <QSize>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#endif

#include "Theme.h"
#include "ThemePicker.h"

namespace Gui {
namespace LinuxCAD {

namespace {

// Role keys are read by Phase 2's QSS, so spell them once here.
const char* kRoleProperty = "linuxcadRole";

struct ThemeEntry
{
    QString id;
    QString name;
    QString author;
    QString description;
    QString bg;
    QString surface;
    QString accent;
    QString text;
};

QList<ThemeEntry> fallbackEntries()
{
    QList<ThemeEntry> entries;
    entries.append({QStringLiteral("charcoal-amber"),
                    QStringLiteral("Charcoal Amber"),
                    QStringLiteral("LinuxCAD"),
                    QStringLiteral("#14171C"),
                    QStringLiteral("#1B1F25"),
                    QStringLiteral("#F59E0B"),
                    QStringLiteral("#ECEEF1")});
    entries.append({QStringLiteral("light-amber"),
                    QStringLiteral("Light Amber"),
                    QStringLiteral("LinuxCAD"),
                    QStringLiteral("#FFFFFF"),
                    QStringLiteral("#F4F6F8"),
                    QStringLiteral("#F59E0B"),
                    QStringLiteral("#1A1F26")});
    entries.append({QStringLiteral("system"),
                    QStringLiteral("System"),
                    QStringLiteral("LinuxCAD"),
                    QStringLiteral("#888888"),
                    QStringLiteral("#888888"),
                    QStringLiteral("#888888"),
                    QStringLiteral("#888888")});
    return entries;
}

QFrame* makeSwatch(const QString& hex)
{
    auto* sw = new QFrame;
    sw->setFixedSize(24, 24);
    sw->setProperty(kRoleProperty, QStringLiteral("theme-picker-swatch"));
    sw->setProperty("linuxcadSwatchHex", hex);
    sw->setFrameShape(QFrame::NoFrame);
    sw->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid #2C323B; border-radius: 3px;")
                          .arg(hex));
    return sw;
}

ThemeEntry fromRecord(const Theme::ThemeRecord& rec)
{
    ThemeEntry e;
    e.id = rec.id;
    e.name = rec.name.isEmpty() ? rec.id : rec.name;
    e.author = rec.author.isEmpty() ? QStringLiteral("Unknown") : rec.author;
    e.description = rec.description;
    e.bg = rec.colors.value(QStringLiteral("bg_primary"), QStringLiteral("#14171C"));
    e.surface = rec.colors.value(QStringLiteral("bg_secondary"), QStringLiteral("#1B1F25"));
    e.accent = rec.colors.value(QStringLiteral("accent"), QStringLiteral("#F59E0B"));
    e.text = rec.colors.value(QStringLiteral("text_primary"), QStringLiteral("#ECEEF1"));
    return e;
}

QWidget* buildRowWidget(const ThemeEntry& e)
{
    auto* row = new QWidget;
    row->setProperty(kRoleProperty, QStringLiteral("theme-picker-row"));
    row->setMinimumHeight(48);

    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(12, 4, 12, 4);
    layout->setSpacing(8);

    auto* labels = new QVBoxLayout;
    labels->setContentsMargins(0, 0, 0, 0);
    labels->setSpacing(2);

    auto* nameLabel = new QLabel(e.name);
    {
        QFont f = nameLabel->font();
        f.setPointSize(14);
        f.setWeight(QFont::DemiBold);
        nameLabel->setFont(f);
    }

    auto* authorLabel = new QLabel(e.author);
    {
        QFont f = authorLabel->font();
        f.setPointSize(12);
        f.setWeight(QFont::Normal);
        authorLabel->setFont(f);
        authorLabel->setStyleSheet(QStringLiteral("color: #A8AEB8;"));
    }

    labels->addWidget(nameLabel);
    labels->addWidget(authorLabel);

    if (!e.description.isEmpty()) {
        auto* descLabel = new QLabel(e.description);
        QFont f = descLabel->font();
        f.setPointSize(10);
        descLabel->setFont(f);
        descLabel->setStyleSheet(QStringLiteral("color: #7B8390;"));
        labels->addWidget(descLabel);
    }

    layout->addLayout(labels, /*stretch=*/1);

    layout->addWidget(makeSwatch(e.bg));
    layout->addWidget(makeSwatch(e.surface));
    layout->addWidget(makeSwatch(e.accent));
    layout->addWidget(makeSwatch(e.text));

    return row;
}

} // namespace

ThemePicker::ThemePicker(QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("LinuxCadThemePicker"));
    setProperty(kRoleProperty, QStringLiteral("theme-picker"));
    setWindowTitle(tr("Themes"));
    setModal(true);
    resize(480, 420);

    buildUi();
    refresh();
}

ThemePicker::~ThemePicker() = default;

void ThemePicker::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(8);

    auto* header = new QLabel(tr("Choose a theme"), this);
    {
        QFont f = header->font();
        f.setPointSize(16);
        f.setWeight(QFont::DemiBold);
        header->setFont(f);
        header->setStyleSheet(QStringLiteral("color: #ECEEF1;"));
    }
    header->setProperty(kRoleProperty, QStringLiteral("theme-picker-header"));
    root->addWidget(header);

    list_ = new QListWidget(this);
    list_->setProperty(kRoleProperty, QStringLiteral("theme-picker-list"));
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setUniformItemSizes(false);
    list_->setSpacing(2);
    root->addWidget(list_, /*stretch=*/1);

    auto* buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->setSpacing(8);
    buttons->addStretch(1);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    closeBtn->setProperty(kRoleProperty, QStringLiteral("theme-picker-close"));
    closeBtn->setAutoDefault(false);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto* applyBtn = new QPushButton(tr("Apply"), this);
    applyBtn->setProperty(kRoleProperty, QStringLiteral("theme-picker-apply"));
    applyBtn->setDefault(true);
    connect(applyBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem* item = list_ ? list_->currentItem() : nullptr;
        if (!item) {
            return;
        }
        const QString id = item->data(Qt::UserRole).toString();
        if (id.isEmpty()) {
            return;
        }
        Q_EMIT themeChosen(id);
    });

    buttons->addWidget(closeBtn);
    buttons->addWidget(applyBtn);
    root->addLayout(buttons);
}

void ThemePicker::refresh()
{
    if (!list_) {
        return;
    }

    list_->clear();

    QList<ThemeEntry> entries;
    const QList<Theme::ThemeRecord> discovered = Theme::discoveredThemes();
    if (discovered.isEmpty()) {
        entries = fallbackEntries();
    }
    else {
        for (const Theme::ThemeRecord& rec : discovered) {
            entries.append(fromRecord(rec));
        }
    }

    QString selectedId = Theme::normalizeThemeId(
        QSettings().value(QStringLiteral("LinuxCAD/Theme"), Theme::defaultThemeId()).toString());
    int selectedRow = -1;
    for (int i = 0; i < entries.size(); ++i) {
        const ThemeEntry& e = entries.at(i);
        auto* item = new QListWidgetItem(list_);
        item->setData(Qt::UserRole, e.id);
        item->setSizeHint(QSize(0, 52));

        QWidget* row = buildRowWidget(e);
        list_->setItemWidget(item, row);

        if (e.id.compare(selectedId, Qt::CaseInsensitive) == 0) {
            selectedRow = i;
        }
    }

    if (selectedRow >= 0) {
        list_->setCurrentRow(selectedRow);
    }
    else if (list_->count() > 0) {
        list_->setCurrentRow(0);
    }
}

} // namespace LinuxCAD
} // namespace Gui
