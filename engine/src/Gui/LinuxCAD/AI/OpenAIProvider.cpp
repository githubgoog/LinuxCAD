// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QString>
#include <QUrl>
#endif

#include <Base/Console.h>

#include "OpenAIProvider.h"

namespace Gui {
namespace LinuxCAD {

namespace {

const QString kDefaultEndpoint = QStringLiteral("https://api.openai.com/v1");
const QString kDefaultModel    = QStringLiteral("gpt-4o-mini");

QJsonArray messagesToJson(const std::vector<Provider::Message>& messages)
{
    QJsonArray arr;
    for (const auto& m : messages) {
        QJsonObject o;
        o.insert(QStringLiteral("role"), m.role);
        o.insert(QStringLiteral("content"), m.content);
        arr.append(o);
    }
    return arr;
}

} // namespace

OpenAIProvider::OpenAIProvider(QObject* parent)
    : Provider(parent)
    , net_(new QNetworkAccessManager(this))
{
}

OpenAIProvider::~OpenAIProvider() = default;

QString OpenAIProvider::displayName() const
{
    return QStringLiteral("OpenAI / OpenAI-compatible");
}

QString OpenAIProvider::readEndpoint() const
{
    QSettings s;
    return s.value(QString::fromLatin1(kSettingEndpoint), kDefaultEndpoint).toString();
}

QString OpenAIProvider::readModel() const
{
    QSettings s;
    return s.value(QString::fromLatin1(kSettingModel), kDefaultModel).toString();
}

QString OpenAIProvider::readApiKey() const
{
    QSettings s;
    return s.value(QString::fromLatin1(kSettingApiKey)).toString();
}

bool OpenAIProvider::isConfigured() const
{
    QSettings s;
    if (!s.value(QString::fromLatin1(kSettingEnabled), false).toBool()) {
        return false;
    }
    if (!s.value(QString::fromLatin1(kSettingConsentGiven), false).toBool()) {
        return false;
    }
    if (readApiKey().isEmpty()) {
        // Local OpenAI-compatible servers (Ollama) often need no key. Allow
        // empty key only when endpoint isn't the default OpenAI URL.
        return readEndpoint() != kDefaultEndpoint;
    }
    return true;
}

bool OpenAIProvider::inFlight() const
{
    return reply_ != nullptr;
}

void OpenAIProvider::cancel()
{
    if (reply_ != nullptr) {
        reply_->abort();
        reply_->deleteLater();
        reply_.clear();
        Q_EMIT stateChanged();
    }
}

void OpenAIProvider::complete(const std::vector<Message>& messages,
                              const QJsonArray& toolSchemas)
{
    if (!isConfigured()) {
        Response r;
        r.ok = false;
        r.errorMessage = QStringLiteral("AI provider not configured");
        Q_EMIT responded(r);
        return;
    }

    cancel(); // ensure no overlap

    QJsonObject body;
    body.insert(QStringLiteral("model"), readModel());
    body.insert(QStringLiteral("messages"), messagesToJson(messages));
    body.insert(QStringLiteral("temperature"), 0.2);
    if (!toolSchemas.isEmpty()) {
        body.insert(QStringLiteral("tools"), toolSchemas);
        body.insert(QStringLiteral("tool_choice"), QStringLiteral("auto"));
    }

    QString endpoint = readEndpoint();
    if (endpoint.endsWith(QLatin1Char('/'))) {
        endpoint.chop(1);
    }
    const QUrl url(endpoint + QStringLiteral("/chat/completions"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");
    const QString key = readApiKey();
    if (!key.isEmpty()) {
        req.setRawHeader("Authorization",
                         QByteArray("Bearer ") + key.toUtf8());
    }

    reply_ = net_->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply_, &QNetworkReply::finished, this, &OpenAIProvider::onFinished);
    Q_EMIT stateChanged();
}

void OpenAIProvider::onFinished()
{
    Response r;
    if (reply_ == nullptr) {
        r.ok = false;
        r.errorMessage = QStringLiteral("Reply went away before completion");
        Q_EMIT responded(r);
        Q_EMIT stateChanged();
        return;
    }

    QNetworkReply* reply = reply_.data();
    reply_.clear();

    const QByteArray raw = reply->readAll();
    const QString errStr = reply->errorString();
    const auto netErr = reply->error();
    reply->deleteLater();

    if (netErr != QNetworkReply::NoError) {
        r.ok = false;
        r.errorMessage = errStr.isEmpty()
            ? QStringLiteral("Network error %1").arg(static_cast<int>(netErr))
            : errStr;
        Base::Console().warning("LinuxCAD AI: OpenAI request failed: %s\n",
                                 r.errorMessage.toUtf8().constData());
        Q_EMIT responded(r);
        Q_EMIT stateChanged();
        return;
    }

    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        r.ok = false;
        r.errorMessage = QStringLiteral("Could not parse AI response: %1").arg(perr.errorString());
        Q_EMIT responded(r);
        Q_EMIT stateChanged();
        return;
    }

    const QJsonObject obj = doc.object();
    if (obj.contains(QStringLiteral("error"))) {
        r.ok = false;
        r.errorMessage = obj.value(QStringLiteral("error")).toObject()
                              .value(QStringLiteral("message")).toString();
        if (r.errorMessage.isEmpty()) {
            r.errorMessage = QStringLiteral("AI provider returned an error.");
        }
        Q_EMIT responded(r);
        Q_EMIT stateChanged();
        return;
    }

    const QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        r.ok = true; // protocol-success but no completion
        Q_EMIT responded(r);
        Q_EMIT stateChanged();
        return;
    }

    const QJsonObject choice = choices.first().toObject();
    const QJsonObject message = choice.value(QStringLiteral("message")).toObject();
    r.text = message.value(QStringLiteral("content")).toString();

    const QJsonArray toolCalls = message.value(QStringLiteral("tool_calls")).toArray();
    for (const auto& v : toolCalls) {
        const QJsonObject tc = v.toObject();
        ToolCall call;
        call.id = tc.value(QStringLiteral("id")).toString();
        const QJsonObject fn = tc.value(QStringLiteral("function")).toObject();
        call.name = fn.value(QStringLiteral("name")).toString();
        const QString argsRaw = fn.value(QStringLiteral("arguments")).toString();
        QJsonParseError aerr{};
        const QJsonDocument adoc = QJsonDocument::fromJson(argsRaw.toUtf8(), &aerr);
        if (aerr.error == QJsonParseError::NoError && adoc.isObject()) {
            call.arguments = adoc.object();
        }
        r.toolCalls.push_back(call);
    }

    r.ok = true;
    Q_EMIT responded(r);
    Q_EMIT stateChanged();
}

} // namespace LinuxCAD
} // namespace Gui
