// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#endif

#include "Consent.h"
#include "Provider.h"

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr const char* kPromptedKey = "LinuxCAD/AI/ConsentPromptedV1";

} // namespace

bool Consent::isGranted()
{
    QSettings s;
    return s.value(QString::fromLatin1(Provider::kSettingConsentGiven), false).toBool();
}

bool Consent::wasPrompted()
{
    QSettings s;
    return s.value(QString::fromLatin1(kPromptedKey), false).toBool();
}

bool Consent::isDocOptedOut(const char* docName)
{
    if (docName == nullptr || docName[0] == '\0') {
        return false;
    }
    QSettings s;
    const QStringList opts = s.value(QString::fromLatin1(Provider::kSettingPerDocOptOut))
                                  .toStringList();
    return opts.contains(QString::fromUtf8(docName));
}

void Consent::setDocOptedOut(const char* docName, bool optedOut)
{
    if (docName == nullptr || docName[0] == '\0') {
        return;
    }
    QSettings s;
    QStringList opts = s.value(QString::fromLatin1(Provider::kSettingPerDocOptOut))
                                  .toStringList();
    const QString name = QString::fromUtf8(docName);
    if (optedOut) {
        if (!opts.contains(name)) {
            opts.append(name);
        }
    }
    else {
        opts.removeAll(name);
    }
    s.setValue(QString::fromLatin1(Provider::kSettingPerDocOptOut), opts);
}

void Consent::promptIfNeeded(QWidget* parent)
{
    if (wasPrompted()) {
        return;
    }

    QDialog dlg(parent);
    dlg.setWindowTitle(QObject::tr("LinuxCAD AI assistant"));
    dlg.setModal(true);
    auto* outer = new QVBoxLayout(&dlg);

    auto* head = new QLabel(QObject::tr("<b>Enable inline AI suggestions?</b>"), &dlg);
    head->setTextFormat(Qt::RichText);
    outer->addWidget(head);

    auto* body = new QLabel(
        QObject::tr(
            "LinuxCAD can offer one-tap suggestions while you model, powered "
            "by your chosen AI provider (OpenAI, Anthropic, or any "
            "OpenAI-compatible endpoint such as Ollama).\n\n"
            "What is sent each time:\n"
            "  • A compact JSON snapshot of your active document - object "
            "names + types, current selection, last few operations.\n"
            "  • No geometry data is uploaded.\n"
            "  • No file contents leave your machine.\n\n"
            "You can disable AI any time, opt a single document out, or "
            "redact object names in Preferences."),
        &dlg);
    body->setWordWrap(true);
    outer->addWidget(body);

    auto* footer = new QLabel(
        QObject::tr("You bring your own API key - LinuxCAD never proxies "
                    "requests through our servers."),
        &dlg);
    footer->setStyleSheet(QStringLiteral("color: #5BA8F2;"));
    footer->setWordWrap(true);
    outer->addWidget(footer);

    auto* buttons = new QDialogButtonBox(&dlg);
    auto* declineBtn = buttons->addButton(QObject::tr("No thanks"),
                                          QDialogButtonBox::RejectRole);
    auto* acceptBtn  = buttons->addButton(QObject::tr("Enable AI"),
                                          QDialogButtonBox::AcceptRole);
    Q_UNUSED(declineBtn);
    Q_UNUSED(acceptBtn);
    outer->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    QSettings s;
    if (dlg.exec() == QDialog::Accepted) {
        s.setValue(QString::fromLatin1(Provider::kSettingConsentGiven), true);
        s.setValue(QString::fromLatin1(Provider::kSettingEnabled),       true);
    }
    else {
        s.setValue(QString::fromLatin1(Provider::kSettingConsentGiven), false);
        s.setValue(QString::fromLatin1(Provider::kSettingEnabled),       false);
    }
    s.setValue(QString::fromLatin1(kPromptedKey), true);
}

} // namespace LinuxCAD
} // namespace Gui
