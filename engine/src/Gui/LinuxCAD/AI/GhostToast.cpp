// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QJsonValue>
#include <QKeyEvent>
#include <QLabel>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QShortcut>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#endif

#include <Base/Console.h>
#include <Gui/MainWindow.h>

#include "GhostToast.h"
#include "ToolRegistry.h"

namespace Gui {
namespace LinuxCAD {

namespace {

QString humanToolTitle(const QString& name)
{
    if (name == QStringLiteral("add_fillet"))    return QObject::tr("Fillet edge");
    if (name == QStringLiteral("add_chamfer"))   return QObject::tr("Chamfer edge");
    if (name == QStringLiteral("add_pad"))       return QObject::tr("Pad sketch");
    if (name == QStringLiteral("add_pocket"))    return QObject::tr("Pocket sketch");
    if (name == QStringLiteral("linear_pattern"))return QObject::tr("Linear pattern");
    if (name == QStringLiteral("polar_pattern")) return QObject::tr("Polar pattern");
    if (name == QStringLiteral("mirror_feature"))return QObject::tr("Mirror feature");
    if (name == QStringLiteral("new_sketch"))    return QObject::tr("New sketch");
    return name;
}

} // namespace

GhostToast::GhostToast(QWidget* parent)
    : QFrame(parent, Qt::ToolTip | Qt::FramelessWindowHint)
    , anchor_(parent)
{
    setObjectName(QStringLiteral("LinuxCadGhostToast"));
    setProperty("linuxcadRole", QStringLiteral("ai-toast"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumWidth(320);
    setMaximumWidth(420);
    hide();

    buildUi();

    fxOpacity_ = new QGraphicsOpacityEffect(this);
    fxOpacity_->setOpacity(0.0);
    setGraphicsEffect(fxOpacity_);
    fxFadeIn_  = new QPropertyAnimation(fxOpacity_, "opacity", this);
    fxFadeIn_->setDuration(180);
    fxFadeIn_->setStartValue(0.0);
    fxFadeIn_->setEndValue(1.0);
    fxFadeIn_->setEasingCurve(QEasingCurve::OutCubic);
    fxFadeOut_ = new QPropertyAnimation(fxOpacity_, "opacity", this);
    fxFadeOut_->setDuration(140);
    fxFadeOut_->setStartValue(1.0);
    fxFadeOut_->setEndValue(0.0);
    fxFadeOut_->setEasingCurve(QEasingCurve::InCubic);
    connect(fxFadeOut_, &QPropertyAnimation::finished, this, &QWidget::hide);

    if (anchor_ != nullptr) {
        anchor_->installEventFilter(this);
    }

    tabAccept_ = new QShortcut(QKeySequence(Qt::Key_Tab), this);
    tabAccept_->setContext(Qt::WidgetWithChildrenShortcut);
    connect(tabAccept_, &QShortcut::activated, this, &GhostToast::onAccept);

    escDismiss_ = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escDismiss_->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escDismiss_, &QShortcut::activated, this, &GhostToast::onDismiss);
}

GhostToast::~GhostToast() = default;

void GhostToast::buildUi()
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(14, 12, 14, 12);
    outer->setSpacing(6);

    titleLabel_ = new QLabel(this);
    titleLabel_->setProperty("linuxcadRole", QStringLiteral("ai-toast-title"));
    titleLabel_->setWordWrap(true);
    outer->addWidget(titleLabel_);

    whyLabel_ = new QLabel(this);
    whyLabel_->setProperty("linuxcadRole", QStringLiteral("ai-toast-sub"));
    whyLabel_->setWordWrap(true);
    outer->addWidget(whyLabel_);

    row_ = new QHBoxLayout();
    row_->setContentsMargins(0, 4, 0, 0);
    row_->setSpacing(8);

    auto* hint = new QLabel(tr("Tab to accept   ·   Esc to dismiss"), this);
    hint->setProperty("linuxcadRole", QStringLiteral("ai-toast-why"));
    row_->addWidget(hint, 1);

    dismissBtn_ = new QToolButton(this);
    dismissBtn_->setText(tr("Dismiss"));
    dismissBtn_->setProperty("linuxcadRole", QStringLiteral("ai-toast-button"));
    connect(dismissBtn_, &QToolButton::clicked, this, &GhostToast::onDismiss);
    row_->addWidget(dismissBtn_);

    acceptBtn_ = new QToolButton(this);
    acceptBtn_->setText(tr("Accept"));
    acceptBtn_->setProperty("linuxcadRole", QStringLiteral("ai-toast-button"));
    acceptBtn_->setProperty("primary", true);
    connect(acceptBtn_, &QToolButton::clicked, this, &GhostToast::onAccept);
    row_->addWidget(acceptBtn_);

    outer->addLayout(row_);
}

QString GhostToast::humanizeArgs(const QJsonObject& args) const
{
    QStringList parts;
    if (args.contains(QStringLiteral("object"))) {
        parts << args.value(QStringLiteral("object")).toString();
    }
    if (args.contains(QStringLiteral("radius"))) {
        parts << QStringLiteral("r=%1mm").arg(args.value(QStringLiteral("radius")).toDouble());
    }
    if (args.contains(QStringLiteral("size"))) {
        parts << QStringLiteral("size=%1mm").arg(args.value(QStringLiteral("size")).toDouble());
    }
    if (args.contains(QStringLiteral("length"))) {
        parts << QStringLiteral("length=%1mm").arg(args.value(QStringLiteral("length")).toDouble());
    }
    if (args.contains(QStringLiteral("depth"))) {
        parts << QStringLiteral("depth=%1mm").arg(args.value(QStringLiteral("depth")).toDouble());
    }
    if (args.contains(QStringLiteral("count"))) {
        parts << QStringLiteral("x%1").arg(args.value(QStringLiteral("count")).toInt());
    }
    if (args.contains(QStringLiteral("plane"))) {
        parts << args.value(QStringLiteral("plane")).toString();
    }
    return parts.join(QStringLiteral("  ·  "));
}

void GhostToast::present(const QString& toolName,
                         const QJsonObject& arguments,
                         const QString& reason)
{
    currentToolName_ = toolName;
    currentArgs_     = arguments;

    const QString title = humanToolTitle(toolName);
    const QString detail = humanizeArgs(arguments);
    if (titleLabel_ != nullptr) {
        if (detail.isEmpty()) {
            titleLabel_->setText(title);
        }
        else {
            titleLabel_->setText(QStringLiteral("%1 — %2").arg(title, detail));
        }
    }
    if (whyLabel_ != nullptr) {
        whyLabel_->setText(reason.trimmed());
    }

    adjustSize();
    reposition();
    show();
    raise();
    activateWindow();
    setFocus();

    if (fxFadeOut_ != nullptr) {
        fxFadeOut_->stop();
    }
    if (fxFadeIn_ != nullptr) {
        fxFadeIn_->stop();
        fxFadeIn_->start();
    }
}

void GhostToast::dismiss()
{
    if (!isVisible()) {
        return;
    }
    if (fxFadeOut_ != nullptr) {
        fxFadeIn_->stop();
        fxFadeOut_->start();
    }
    else {
        hide();
    }
}

void GhostToast::onDismiss()
{
    currentToolName_.clear();
    currentArgs_ = QJsonObject();
    dismiss();
}

void GhostToast::onAccept()
{
    if (currentToolName_.isEmpty()) {
        dismiss();
        return;
    }
    auto result = ToolRegistry::instance().execute(currentToolName_, currentArgs_);
    if (!result.ok) {
        Base::Console().log("LinuxCAD AI: tool '%s' failed: %s\n",
                             currentToolName_.toUtf8().constData(),
                             result.message.toUtf8().constData());
    }
    onDismiss();
}

void GhostToast::reposition()
{
    auto* p = anchor_.data();
    if (p == nullptr) {
        p = Gui::getMainWindow();
        anchor_ = p;
        if (p != nullptr) {
            p->installEventFilter(this);
        }
    }
    if (p == nullptr) {
        return;
    }
    constexpr int kInset = 24;
    adjustSize();
    const QPoint anchorBR = p->mapToGlobal(QPoint(p->width(), p->height()));
    move(anchorBR.x() - width() - kInset, anchorBR.y() - height() - kInset);
}

bool GhostToast::event(QEvent* ev)
{
    if (ev != nullptr && ev->type() == QEvent::Show) {
        reposition();
    }
    return QFrame::event(ev);
}

bool GhostToast::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev != nullptr && obj == anchor_) {
        if (ev->type() == QEvent::Resize || ev->type() == QEvent::Move) {
            if (isVisible()) {
                reposition();
            }
        }
    }
    return QFrame::eventFilter(obj, ev);
}

void GhostToast::keyPressEvent(QKeyEvent* ev)
{
    if (ev != nullptr) {
        if (ev->key() == Qt::Key_Tab) {
            onAccept();
            return;
        }
        if (ev->key() == Qt::Key_Escape) {
            onDismiss();
            return;
        }
    }
    QFrame::keyPressEvent(ev);
}

} // namespace LinuxCAD
} // namespace Gui
