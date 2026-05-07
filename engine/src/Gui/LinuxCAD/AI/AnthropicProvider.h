// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_ANTHROPIC_PROVIDER_H
#define GUI_LINUXCAD_AI_ANTHROPIC_PROVIDER_H

#include <FCGlobal.h>
#include <QPointer>

#include "Provider.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace Gui {
namespace LinuxCAD {

/// Implements `Provider` against Anthropic's Messages API
/// (`POST {endpoint}/v1/messages`). Tool calls come back as `tool_use` blocks
/// inside the assistant message.
class GuiExport AnthropicProvider : public Provider
{
    Q_OBJECT

public:
    explicit AnthropicProvider(QObject* parent = nullptr);
    ~AnthropicProvider() override;

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

    QNetworkAccessManager*       net_ = nullptr;
    QPointer<QNetworkReply>      reply_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_ANTHROPIC_PROVIDER_H
