// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_GHOSTTOAST_H
#define GUI_LINUXCAD_AI_GHOSTTOAST_H

#include <FCGlobal.h>
#include <QFrame>
#include <QJsonObject>
#include <QPointer>
#include <QString>

class QGraphicsOpacityEffect;
class QHBoxLayout;
class QLabel;
class QPropertyAnimation;
class QShortcut;
class QToolButton;
class QVBoxLayout;

namespace Gui {
namespace LinuxCAD {

/// Bottom-right, frameless ghost suggestion widget.
///
/// Shows an AI-suggested tool call with:
///   * tool name humanized ("Fillet 2 mm on Edge3")
///   * a short "why" line
///   * Tab = accept (executes through ToolRegistry inside a transaction),
///     Esc = dismiss
///
/// The widget never modifies the document until the user accepts. Accept
/// runs the ToolRegistry tool which itself wraps the change in
/// `App::Document::openTransaction(...)`, so a single Ctrl+Z fully reverts.
class GuiExport GhostToast : public QFrame
{
    Q_OBJECT

public:
    explicit GhostToast(QWidget* parent = nullptr);
    ~GhostToast() override;

    /// Show a new suggestion. Replaces any pending one.
    void present(const QString& toolName,
                 const QJsonObject& arguments,
                 const QString& reason);

    /// Hide and clear current suggestion.
    void dismiss();

protected:
    void keyPressEvent(QKeyEvent* ev) override;
    bool event(QEvent* ev) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

private Q_SLOTS:
    void onAccept();
    void onDismiss();

private:
    void buildUi();
    void reposition();
    QString humanizeArgs(const QJsonObject& args) const;

    QPointer<QWidget>            anchor_;
    QHBoxLayout*                 row_         = nullptr;
    QLabel*                      titleLabel_  = nullptr;
    QLabel*                      whyLabel_    = nullptr;
    QToolButton*                 acceptBtn_   = nullptr;
    QToolButton*                 dismissBtn_  = nullptr;
    QGraphicsOpacityEffect*      fxOpacity_   = nullptr;
    QPropertyAnimation*          fxFadeIn_    = nullptr;
    QPropertyAnimation*          fxFadeOut_   = nullptr;
    QShortcut*                   tabAccept_   = nullptr;
    QShortcut*                   escDismiss_  = nullptr;

    QString     currentToolName_;
    QJsonObject currentArgs_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_GHOSTTOAST_H
