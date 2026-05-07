// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QString>
#include <QUrl>
#endif

#include <Base/Console.h>

#include "AnthropicProvider.h"

namespace Gui {
namespace LinuxCAD {

namespace {

const QString kDefaultEndpoint = QStringLiteral("https://api.anthropic.com");
const QString kDefaultModel    = QStringLiteral("claude-3-5-haiku-latest");

} // namespace

AnthropicProvider::AnthropicProvider(QObject* parent)
    : Provider(parent)
    , net_(new QNetworkAccessManager(this))
{
}

AnthropicProvider::~AnthropicProvider() = default;

QString AnthropicProvider::displayName() const
{
    return QStringLiteral("Anthropic Claude");
}

QString AnthropicProvider::readEndpoint() const
{
    QSettings s;
    return s.value(QString::fromLatin1(kSettingEndpoint), kDefaultEndpoint).toString();
}

QString AnthropicProvider::readModel() const
{
    QSettings s;
    return s.value(QString::fromLatin1(kSettingModel), kDefaultModel).toString();
}

QString AnthropicProvider::readApiKey() const
{
    QSettings s;
    return s.value(QString::fromLatin1(kSettingApiKey)).toString();
}

bool AnthropicProvider::isConfigured() const
{
    QSettings s;
    if (!s.value(QString::fromLatin1(kSettingEnabled), false).toBool()) {
        return false;
    }
    if (!s.value(QString::fromLatin1(kSettingConsentGiven), false).toBool()) {
        return false;
    }
    return !readApiKey().isEmpty();
}

bool AnthropicProvider::inFlight() const
{
    return reply_ != nullptr;
}

void AnthropicProvider::cancel()
{
    if (reply_ != nullptr) {
        reply_->abort();
        reply_->deleteLater();
        reply_.clear();
        Q_EMIT stateChanged();
    }
}

void AnthropicProvider::complete(const std::vector<Message>& messages,
                                 const QJsonArray& toolSchemas)
{
    if (!isConfigured()) {
        Response r;
        r.ok = false;
        r.errorMessage = QStringLiteral("Anthropic provider not configured");
        Q_EMIT responded(r);
        return;
    }

    cancel();

    // Anthropic separates the system prompt from the conversation array.
    QString systemPrompt;
    QJsonArray conv;
    for (const auto& m : messages) {
        if (m.role == QStringLiteral("system")) {
            if (!systemPrompt.isEmpty()) {
                systemPrompt += QStringLiteral("\n\n");
            }
            systemPrompt += m.content;
            continue;
        }
        QJsonObject obj;
        obj.insert(QStringLiteral("role"),
                   m.role == QStringLiteral("assistant") ? QStringLiteral("assistant")
                                                         : QStringLiteral("user"));
        QJsonArray content;
        QJsonObject blk;
        blk.insert(QStringLiteral("type"), QStringLiteral("text"));
        blk.insert(QStringLiteral("text"), m.content);
        content.append(blk);
        obj.insert(QStringLiteral("content"), content);
        conv.append(obj);
    }

    QJsonObject body;
    body.insert(QStringLiteral("model"), readModel());
    body.insert(QStringLiteral("messages"), conv);
    body.insert(QStringLiteral("max_tokens"), 1024);
    if (!systemPrompt.isEmpty()) {
        body.insert(QStringLiteral("system"), systemPrompt);
    }
    if (!toolSchemas.isEmpty()) {
        // Anthropic's tool schema differs slightly from OpenAI's. We accept
        // OpenAI-style schemas and flatten them to Anthropic's shape:
        //   {name, description, input_schema} instead of
        //   {function:{name, description, parameters}}
        QJsonArray atools;
        for (const auto& v : toolSchemas) {
            const QJsonObject openaiTool = v.toObject();
            const QJsonObject fn = openaiTool.value(QStringLiteral("function")).toObject();
            QJsonObject t;
            t.insert(QStringLiteral("name"), fn.value(QStringLiteral("name")));
            t.insert(QStringLiteral("description"), fn.value(QStringLiteral("description")));
            t.insert(QStringLiteral("input_schema"), fn.value(QStringLiteral("parameters")));
            atools.append(t);
        }
        body.insert(QStringLiteral("tools"), atools);
    }

    QString endpoint = readEndpoint();
    if (endpoint.endsWith(QLatin1Char('/'))) {
        endpoint.chop(1);
    }
    const QUrl url(endpoint + QStringLiteral("/v1/messages"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("anthropic-version", "2023-06-01");
    const QString key = readApiKey();
    req.setRawHeader("x-api-key", key.toUtf8());

    reply_ = net_->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply_, &QNetworkReply::finished, this, &AnthropicProvider::onFinished);
    Q_EMIT stateChanged();
}

void AnthropicProvider::onFinished()
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
        Base::Console().warning("LinuxCAD AI: Anthropic request failed: %s\n",
                                 r.errorMessage.toUtf8().constData());
        Q_EMIT responded(r);
        Q_EMIT stateChanged();
        return;
    }

    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        r.ok = false;
        r.errorMessage = QStringLiteral("Could not parse Anthropic response: %1")
                             .arg(perr.errorString());
        Q_EMIT responded(r);
        Q_EMIT stateChanged();
        return;
    }

    const QJsonObject obj = doc.object();
    if (obj.contains(QStringLiteral("error"))) {
        r.ok = false;
        r.errorMessage = obj.value(QStringLiteral("error")).toObject()
                              .value(QStringLiteral("message")).toString();
        Q_EMIT responded(r);
        Q_EMIT stateChanged();
        return;
    }

    QString text;
    const QJsonArray content = obj.value(QStringLiteral("content")).toArray();
    for (const auto& v : content) {
        const QJsonObject blk = v.toObject();
        const QString type = blk.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("text")) {
            text += blk.value(QStringLiteral("text")).toString();
        }
        else if (type == QStringLiteral("tool_use")) {
            ToolCall call;
            call.id = blk.value(QStringLiteral("id")).toString();
            call.name = blk.value(QStringLiteral("name")).toString();
            call.arguments = blk.value(QStringLiteral("input")).toObject();
            r.toolCalls.push_back(call);
        }
    }
    r.text = text;
    r.ok = true;
    Q_EMIT responded(r);
    Q_EMIT stateChanged();
}

} // namespace LinuxCAD
} // namespace Gui
