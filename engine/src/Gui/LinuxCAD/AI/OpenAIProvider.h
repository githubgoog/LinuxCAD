// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_OPENAI_PROVIDER_H
#define GUI_LINUXCAD_AI_OPENAI_PROVIDER_H

#include <FCGlobal.h>
#include <QPointer>

#include "Provider.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace Gui {
namespace LinuxCAD {

/// Implements `Provider` against the OpenAI REST API
/// (`POST {endpoint}/chat/completions`). Because the same wire format is used
/// by Ollama / OpenRouter / LM Studio / vLLM, this single class also covers
/// "OpenAI-compatible" providers when the user changes the endpoint URL.
class GuiExport OpenAIProvider : public Provider
{
    Q_OBJECT

public:
    explicit OpenAIProvider(QObject* parent = nullptr);
    ~OpenAIProvider() override;

    QString displayName() const override;
    bool    isConfigured() const override;
    void    complete(const std::vector<Message>& messages,
                     const QJsonArray& toolSchemas) override;
    void    cancel() override;
    bool    inFlight() const override;

private Q_SLOTS:
    void onFinished();

private:
    QString readEndpoint() const;
    QString readModel() const;
    QString readApiKey() const;

    QNetworkAccessManager*       net_   = nullptr;
    QPointer<QNetworkReply>      reply_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_OPENAI_PROVIDER_H
