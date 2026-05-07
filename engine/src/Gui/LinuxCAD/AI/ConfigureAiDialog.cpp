// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QComboBox>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <vector>
#endif

#include "ConfigureAiDialog.h"
#include "MockProvider.h"
#include "Provider.h"

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr int kPageProvider    = 0;
constexpr int kPageCredentials = 1;
constexpr int kPageTest        = 2;

} // namespace

ConfigureAiDialog::ConfigureAiDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Configure AI"));
    setModal(true);

    stack_ = new QStackedWidget(this);

    // --- Page 0: provider ---
    auto* pageProvider = new QWidget(this);
    auto* pv           = new QVBoxLayout(pageProvider);
    pv->addWidget(new QLabel(tr("Choose an AI provider:"), pageProvider));
    providerMock_ = new QRadioButton(tr("Mock (offline test)"), pageProvider);
    providerOpenAI_     = new QRadioButton(tr("OpenAI / OpenAI-compatible"), pageProvider);
    providerAnthropic_  = new QRadioButton(tr("Anthropic Claude"), pageProvider);
    auto* radioGroup = new QVBoxLayout();
    radioGroup->addWidget(providerMock_);
    radioGroup->addWidget(providerOpenAI_);
    radioGroup->addWidget(providerAnthropic_);
    pv->addLayout(radioGroup);
    auto* p0btn = new QHBoxLayout();
    auto* p0cancel = new QPushButton(tr("Cancel"), pageProvider);
    auto* p0next   = new QPushButton(tr("Next"), pageProvider);
    p0btn->addStretch(1);
    p0btn->addWidget(p0cancel);
    p0btn->addWidget(p0next);
    pv->addLayout(p0btn);
    connect(p0cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(p0next, &QPushButton::clicked, this, &ConfigureAiDialog::onProviderNext);

    // --- Page 1: credentials ---
    auto* pageCred = new QWidget(this);
    auto* cv       = new QVBoxLayout(pageCred);
    cv->addWidget(new QLabel(tr("API key:"), pageCred));
    apiKeyEdit_ = new QLineEdit(pageCred);
    apiKeyEdit_->setEchoMode(QLineEdit::Password);
    cv->addWidget(apiKeyEdit_);
    cv->addWidget(new QLabel(tr("Model:"), pageCred));
    modelCombo_ = new QComboBox(pageCred);
    cv->addWidget(modelCombo_);
    auto* cbtn = new QHBoxLayout();
    auto* cback = new QPushButton(tr("Back"), pageCred);
    auto* cnext = new QPushButton(tr("Next"), pageCred);
    cbtn->addStretch(1);
    cbtn->addWidget(cback);
    cbtn->addWidget(cnext);
    cv->addLayout(cbtn);
    connect(cback, &QPushButton::clicked, this, &ConfigureAiDialog::onCredentialsBack);
    connect(cnext, &QPushButton::clicked, this, &ConfigureAiDialog::onCredentialsNext);

    // --- Page 2: test + save ---
    auto* pageTest = new QWidget(this);
    auto* tv       = new QVBoxLayout(pageTest);
    testBtn_       = new QPushButton(tr("Run test"), pageTest);
    tv->addWidget(testBtn_);
    testResult_ = new QLabel(pageTest);
    testResult_->setWordWrap(true);
    tv->addWidget(testResult_);
    tv->addStretch(1);
    auto* tbtn = new QHBoxLayout();
    auto* tback = new QPushButton(tr("Back"), pageTest);
    auto* tsave = new QPushButton(tr("Save"), pageTest);
    tbtn->addStretch(1);
    tbtn->addWidget(tback);
    tbtn->addWidget(tsave);
    tv->addLayout(tbtn);
    connect(testBtn_, &QPushButton::clicked, this, &ConfigureAiDialog::onTestClicked);
    connect(tback, &QPushButton::clicked, this, &ConfigureAiDialog::onTestPageBack);
    connect(tsave, &QPushButton::clicked, this, &ConfigureAiDialog::onSaveClicked);

    stack_->addWidget(pageProvider);
    stack_->addWidget(pageCred);
    stack_->addWidget(pageTest);

    auto* outer = new QVBoxLayout(this);
    outer->addWidget(stack_);

    loadSettings();
}

int ConfigureAiDialog::run(QWidget* parent)
{
    ConfigureAiDialog dlg(parent);
    return dlg.exec();
}

void ConfigureAiDialog::loadSettings()
{
    QSettings s;
    const QString p = s.value(QString::fromLatin1(Provider::kSettingProvider),
                              QStringLiteral("mock")).toString();
    if (p.compare(QStringLiteral("anthropic"), Qt::CaseInsensitive) == 0) {
        providerAnthropic_->setChecked(true);
    }
    else if (p.compare(QStringLiteral("openai"), Qt::CaseInsensitive) == 0
             || p.compare(QStringLiteral("openai-compatible"), Qt::CaseInsensitive) == 0) {
        providerOpenAI_->setChecked(true);
    }
    else {
        providerMock_->setChecked(true);
    }

    apiKeyEdit_->setText(s.value(QString::fromLatin1(Provider::kSettingApiKey)).toString());
    refreshModelChoices();
    const QString model = s.value(QString::fromLatin1(Provider::kSettingModel)).toString();
    if (!model.isEmpty()) {
        const int idx = modelCombo_->findText(model);
        if (idx >= 0) {
            modelCombo_->setCurrentIndex(idx);
        }
        else {
            modelCombo_->setCurrentText(model);
        }
    }
}

void ConfigureAiDialog::refreshModelChoices()
{
    const QSignalBlocker blocker(modelCombo_);
    modelCombo_->clear();
    if (providerAnthropic_->isChecked()) {
        modelCombo_->addItem(QStringLiteral("claude-3-5-sonnet-latest"));
        modelCombo_->addItem(QStringLiteral("claude-3-5-haiku-latest"));
    }
    else {
        modelCombo_->addItem(QStringLiteral("gpt-4o-mini"));
        modelCombo_->addItem(QStringLiteral("gpt-4o"));
    }
}

QString ConfigureAiDialog::providerKindToStore() const
{
    if (providerAnthropic_->isChecked()) {
        return QStringLiteral("anthropic");
    }
    if (providerOpenAI_->isChecked()) {
        return QStringLiteral("openai");
    }
    return QStringLiteral("mock");
}

void ConfigureAiDialog::onProviderNext()
{
    if (providerMock_->isChecked()) {
        stack_->setCurrentIndex(kPageTest);
        return;
    }
    refreshModelChoices();
    stack_->setCurrentIndex(kPageCredentials);
}

void ConfigureAiDialog::onCredentialsNext()
{
    stack_->setCurrentIndex(kPageTest);
}

void ConfigureAiDialog::onCredentialsBack()
{
    stack_->setCurrentIndex(kPageProvider);
}

void ConfigureAiDialog::onTestPageBack()
{
    if (providerMock_->isChecked()) {
        stack_->setCurrentIndex(kPageProvider);
        return;
    }
    stack_->setCurrentIndex(kPageCredentials);
}

bool ConfigureAiDialog::runProviderPing(Provider* provider, QString* failMessage)
{
    QEventLoop                loop;
    QTimer                    timeoutTimer;
    bool                      gotResponse = false;
    Provider::Response        response;

    timeoutTimer.setSingleShot(true);

    QObject::connect(provider, &Provider::responded, &loop,
                     [&gotResponse, &response, &loop](const Provider::Response& r) {
                         response    = r;
                         gotResponse = true;
                         loop.quit();
                     });

    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    std::vector<Provider::Message> msgs;
    msgs.push_back({QStringLiteral("user"), QStringLiteral("ping")});
    provider->complete(msgs, QJsonArray());

    timeoutTimer.start(3000);
    loop.exec();

    if (!gotResponse) {
        provider->cancel();
        *failMessage = QObject::tr("Timed out after 3 seconds");
        return false;
    }
    if (!response.ok) {
        *failMessage = response.errorMessage.isEmpty()
            ? QObject::tr("Provider returned an error")
            : response.errorMessage;
        return false;
    }
    return true;
}

void ConfigureAiDialog::onTestClicked()
{
    testResult_->setText(tr("Running test…"));
    testResult_->setStyleSheet(QString());
    QApplication::processEvents();

    QString fail;

    if (providerMock_->isChecked()) {
        MockProvider mock(this);
        if (runProviderPing(&mock, &fail)) {
            testResult_->setText(tr("Pass: provider responded successfully."));
            testResult_->setStyleSheet(QStringLiteral("color: #008000;"));
        }
        else {
            testResult_->setText(tr("Fail: %1").arg(fail));
            testResult_->setStyleSheet(QStringLiteral("color: #c00000;"));
        }
        return;
    }

    QSettings st;
    const QString kProv = QString::fromLatin1(Provider::kSettingProvider);
    const QString kKey  = QString::fromLatin1(Provider::kSettingApiKey);
    const QString kModel = QString::fromLatin1(Provider::kSettingModel);
    const QString kEn    = QString::fromLatin1(Provider::kSettingEnabled);
    const QString kConsent = QString::fromLatin1(Provider::kSettingConsentGiven);

    const QVariant oldProv   = st.value(kProv);
    const QVariant oldKey    = st.value(kKey);
    const QVariant oldModel  = st.value(kModel);
    const QVariant oldEn     = st.value(kEn);
    const QVariant oldConsent = st.value(kConsent);

    st.setValue(kProv, providerKindToStore());
    st.setValue(kKey, apiKeyEdit_->text());
    st.setValue(kModel, modelCombo_->currentText());
    st.setValue(kEn, true);
    st.setValue(kConsent, true);
    st.sync();

    QString failRun;
    bool    ok = false;
    {
        std::unique_ptr<Provider> p(Provider::createFromSettings(nullptr));
        if (!p) {
            failRun = tr("Could not create provider");
        }
        else if (!p->isConfigured()) {
            failRun = tr("Provider not configured (check API key and settings)");
        }
        else {
            ok = runProviderPing(p.get(), &failRun);
        }
    }

    st.setValue(kProv, oldProv);
    st.setValue(kKey, oldKey);
    st.setValue(kModel, oldModel);
    st.setValue(kEn, oldEn);
    st.setValue(kConsent, oldConsent);
    st.sync();

    if (ok) {
        testResult_->setText(tr("Pass: provider responded successfully."));
        testResult_->setStyleSheet(QStringLiteral("color: #008000;"));
    }
    else {
        testResult_->setText(tr("Fail: %1").arg(failRun));
        testResult_->setStyleSheet(QStringLiteral("color: #c00000;"));
    }
}

void ConfigureAiDialog::onSaveClicked()
{
    QSettings st;
    st.setValue(QString::fromLatin1(Provider::kSettingProvider), providerKindToStore());
    if (providerMock_->isChecked()) {
        st.setValue(QString::fromLatin1(Provider::kSettingApiKey), QString());
        st.setValue(QString::fromLatin1(Provider::kSettingModel), QString());
    }
    else {
        st.setValue(QString::fromLatin1(Provider::kSettingApiKey), apiKeyEdit_->text());
        st.setValue(QString::fromLatin1(Provider::kSettingModel), modelCombo_->currentText());
    }
    accept();
}

} // namespace LinuxCAD
} // namespace Gui
