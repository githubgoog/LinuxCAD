// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_CONFIGURE_AI_DIALOG_H
#define GUI_LINUXCAD_AI_CONFIGURE_AI_DIALOG_H

#include <FCGlobal.h>

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QStackedWidget;

namespace Gui {
namespace LinuxCAD {

class Provider;

/// Standalone wizard to pick an AI provider and persist QSettings used by
/// `Provider::createFromSettings`. Callers should reload the live provider
/// after `Accepted`.
class GuiExport ConfigureAiDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigureAiDialog(QWidget* parent = nullptr);

    static int run(QWidget* parent);

private Q_SLOTS:
    void onProviderNext();
    void onCredentialsNext();
    void onCredentialsBack();
    void onTestPageBack();
    void onTestClicked();
    void onSaveClicked();

private:
    void loadSettings();
    void refreshModelChoices();
    QString providerKindToStore() const;

    static bool runProviderPing(Provider* provider, QString* failMessage);

    QStackedWidget* stack_ = nullptr;

    QRadioButton* providerMock_      = nullptr;
    QRadioButton* providerOpenAI_    = nullptr;
    QRadioButton* providerAnthropic_ = nullptr;

    QLineEdit* apiKeyEdit_ = nullptr;
    QComboBox* modelCombo_ = nullptr;

    QPushButton* testBtn_   = nullptr;
    QLabel*      testResult_ = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_CONFIGURE_AI_DIALOG_H
