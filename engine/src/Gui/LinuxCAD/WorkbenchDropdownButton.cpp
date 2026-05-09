// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPixmap>
#include <QPoint>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/WorkbenchManager.h>

#include "WorkbenchDropdownButton.h"

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr int kPopupWidth     = 320;
constexpr int kPopupMaxHeight = 380;

QString chevronGlyph()
{
    return QStringLiteral("\u25BE"); // ▼
}

} // namespace

WorkbenchDropdownButton::WorkbenchDropdownButton(QWidget* parent)
    : QToolButton(parent)
{
    setObjectName(QStringLiteral("workbench-dropdown"));
    setProperty("linuxcadRole", QStringLiteral("workbench-dropdown"));
    setAutoRaise(true);
    setMinimumWidth(200);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    setPopupMode(QToolButton::DelayedPopup); // we drive the popup ourselves

    buildFace();

    connect(this, &QToolButton::clicked, this, &WorkbenchDropdownButton::onClicked);

    // Defensive: keep the face in sync with whatever workbench FreeCAD is on
    // right now (set may not be the same one used at construction time).
    if (auto* app = Gui::Application::Instance) {
        try {
            app->signalActivateWorkbench.connect([this](const char* name) {
                if (name == nullptr) {
                    return;
                }
                workbenchLabel(QString::fromUtf8(name));
            });
        }
        catch (...) {
            // Defensive: never let a signal hookup break the UI.
        }
    }

    syncToActive();
}

WorkbenchDropdownButton::~WorkbenchDropdownButton() = default;

void WorkbenchDropdownButton::buildFace()
{
    faceLayout_ = new QHBoxLayout(this);
    faceLayout_->setContentsMargins(8, 2, 8, 2);
    faceLayout_->setSpacing(6);

    iconLabel_ = new QLabel(this);
    iconLabel_->setFixedSize(QSize(18, 18));
    iconLabel_->setScaledContents(true);
    iconLabel_->setProperty("linuxcadRole", QStringLiteral("workbench-dropdown-icon"));
    faceLayout_->addWidget(iconLabel_);

    textLabel_ = new QLabel(tr("Workbench"), this);
    textLabel_->setProperty("linuxcadRole", QStringLiteral("workbench-dropdown-text"));
    textLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    faceLayout_->addWidget(textLabel_, 1);

    chevronLabel_ = new QLabel(chevronGlyph(), this);
    chevronLabel_->setProperty("linuxcadRole", QStringLiteral("workbench-dropdown-chevron"));
    faceLayout_->addWidget(chevronLabel_);
}

void WorkbenchDropdownButton::ensurePopup()
{
    if (popup_ != nullptr) {
        return;
    }

    popup_ = new QWidget(this, Qt::Popup);
    popup_->setObjectName(QStringLiteral("WorkbenchDropdownButton-popup"));
    popup_->setProperty("linuxcadRole", QStringLiteral("workbench-dropdown-popup"));
    popup_->setFocusPolicy(Qt::StrongFocus);
    popup_->installEventFilter(this);

    auto* col = new QVBoxLayout(popup_);
    col->setContentsMargins(6, 6, 6, 6);
    col->setSpacing(4);

    filter_ = new QLineEdit(popup_);
    filter_->setObjectName(QStringLiteral("workbench-dropdown-filter"));
    filter_->setProperty("linuxcadRole", QStringLiteral("workbench-dropdown-filter"));
    filter_->setPlaceholderText(tr("Filter workbenches..."));
    filter_->setClearButtonEnabled(true);
    filter_->installEventFilter(this);
    col->addWidget(filter_);

    list_ = new QListWidget(popup_);
    list_->setObjectName(QStringLiteral("workbench-dropdown-list"));
    list_->setProperty("linuxcadRole", QStringLiteral("workbench-dropdown-list"));
    list_->setUniformItemSizes(false);
    list_->setIconSize(QSize(20, 20));
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->installEventFilter(this);
    col->addWidget(list_);

    connect(filter_, &QLineEdit::textChanged,
            this,    &WorkbenchDropdownButton::onFilterTextChanged);
    connect(filter_, &QLineEdit::returnPressed, this, [this]() {
        if (list_ == nullptr) {
            return;
        }
        // Activate the first visible item on Enter.
        for (int i = 0; i < list_->count(); ++i) {
            if (auto* it = list_->item(i); it != nullptr && !it->isHidden()) {
                activateRow(it);
                return;
            }
        }
    });
    connect(list_, &QListWidget::itemActivated,
            this,  &WorkbenchDropdownButton::onItemActivated);
    connect(list_, &QListWidget::itemClicked,
            this,  &WorkbenchDropdownButton::onItemActivated);

    popup_->resize(kPopupWidth, kPopupMaxHeight);
}

void WorkbenchDropdownButton::rebuildModel()
{
    if (list_ == nullptr) {
        return;
    }
    auto* app = Gui::Application::Instance;
    list_->clear();
    if (app == nullptr) {
        return;
    }

    QStringList wbs = app->workbenches();
    wbs.sort(Qt::CaseInsensitive);

    for (const QString& wb : wbs) {
        if (wb.compare(QLatin1String("NoneWorkbench"), Qt::CaseInsensitive) == 0) {
            continue;
        }

        QString display = app->workbenchMenuText(wb);
        if (display.isEmpty()) {
            display = wb;
        }
        const QPixmap icon = app->workbenchIcon(wb);
        auto* item = (icon.isNull()) ? new QListWidgetItem(display)
                                     : new QListWidgetItem(QIcon(icon), display);
        // UserRole carries the *internal* workbench name, matching the
        // semantics of the legacy combobox (the same string we hand to
        // activateWorkbench()).
        item->setData(Qt::UserRole, wb);
        list_->addItem(item);
    }
}

void WorkbenchDropdownButton::applyFilter(const QString& text)
{
    if (list_ == nullptr) {
        return;
    }
    const QString needle = text.trimmed();
    int firstVisible = -1;
    for (int i = 0; i < list_->count(); ++i) {
        QListWidgetItem* it = list_->item(i);
        if (it == nullptr) {
            continue;
        }
        const QString display  = it->text();
        const QString internal = it->data(Qt::UserRole).toString();
        const bool match = needle.isEmpty()
            || display.contains(needle,  Qt::CaseInsensitive)
            || internal.contains(needle, Qt::CaseInsensitive);
        it->setHidden(!match);
        if (match && firstVisible < 0) {
            firstVisible = i;
        }
    }
    if (firstVisible >= 0) {
        list_->setCurrentRow(firstVisible);
    }
}

void WorkbenchDropdownButton::positionPopup()
{
    if (popup_ == nullptr) {
        return;
    }
    const QPoint anchor = mapToGlobal(QPoint(0, height()));
    popup_->move(anchor);
    int desiredW = qMax(kPopupWidth, width());
    popup_->resize(desiredW, kPopupMaxHeight);
}

void WorkbenchDropdownButton::onClicked()
{
    ensurePopup();
    rebuildModel();
    if (filter_ != nullptr) {
        filter_->clear();
    }
    applyFilter(QString());
    positionPopup();
    popup_->show();
    popup_->raise();
    popup_->activateWindow();
    if (filter_ != nullptr) {
        filter_->setFocus(Qt::ShortcutFocusReason);
    }
}

void WorkbenchDropdownButton::onFilterTextChanged(const QString& text)
{
    applyFilter(text);
}

void WorkbenchDropdownButton::onItemActivated(QListWidgetItem* item)
{
    activateRow(item);
}

void WorkbenchDropdownButton::activateRow(QListWidgetItem* item)
{
    if (item == nullptr) {
        return;
    }
    const QString wb = item->data(Qt::UserRole).toString();
    if (wb.isEmpty()) {
        return;
    }
    if (popup_ != nullptr) {
        popup_->hide();
    }
    auto* app = Gui::Application::Instance;
    if (app == nullptr) {
        return;
    }
    updatingDropdown_ = true;
    try {
        app->activateWorkbench(wb.toLatin1().constData());
    }
    catch (...) {
        // Defensive: invalid workbench should not propagate into the UI loop.
    }
    updatingDropdown_ = false;
    workbenchLabel(wb);
    Q_EMIT workbenchActivated(wb);
}

void WorkbenchDropdownButton::workbenchLabel(const QString& wb)
{
    if (textLabel_ == nullptr) {
        return;
    }
    auto* app = Gui::Application::Instance;
    QString display;
    if (wb.compare(QLatin1String("NoneWorkbench"), Qt::CaseInsensitive) == 0) {
        display = tr("Select workbench...");
    }
    else if (app != nullptr) {
        display = app->workbenchMenuText(wb);
        if (display.isEmpty()) {
            display = wb;
        }
    }
    else {
        display = wb.isEmpty() ? tr("Workbench") : wb;
    }
    QPixmap icon;
    if (app != nullptr
        && wb.compare(QLatin1String("NoneWorkbench"), Qt::CaseInsensitive) != 0) {
        icon = app->workbenchIcon(wb);
    }
    textLabel_->setText(display);
    if (iconLabel_ != nullptr) {
        if (icon.isNull()) {
            iconLabel_->clear();
        }
        else {
            iconLabel_->setPixmap(icon.scaled(QSize(18, 18),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
        }
    }
    setToolTip(display);
}

void WorkbenchDropdownButton::syncToActive()
{
    QString active;
    try {
        if (auto* mgr = Gui::WorkbenchManager::instance()) {
            active = QString::fromStdString(mgr->activeName());
        }
    }
    catch (...) {
        active.clear();
    }
    if (active.isEmpty()) {
        if (textLabel_ != nullptr) {
            textLabel_->setText(tr("Workbench"));
        }
        return;
    }
    workbenchLabel(active);
}

bool WorkbenchDropdownButton::eventFilter(QObject* watched, QEvent* event)
{
    if (popup_ != nullptr && event != nullptr) {
        if (event->type() == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Escape) {
                popup_->hide();
                return true;
            }
            // Pressing Down from the filter should focus the list.
            if (watched == filter_ && list_ != nullptr
                && (ke->key() == Qt::Key_Down || ke->key() == Qt::Key_Up)) {
                list_->setFocus(Qt::ShortcutFocusReason);
                if (list_->currentRow() < 0 && list_->count() > 0) {
                    list_->setCurrentRow(0);
                }
                return true;
            }
        }
    }
    return QToolButton::eventFilter(watched, event);
}

} // namespace LinuxCAD
} // namespace Gui
