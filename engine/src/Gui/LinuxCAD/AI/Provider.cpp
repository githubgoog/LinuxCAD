// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QSettings>
#include <QString>
#endif

#include "Provider.h"
#include "OpenAIProvider.h"
#include "AnthropicProvider.h"

namespace Gui {
namespace LinuxCAD {

Provider::Provider(QObject* parent)
    : QObject(parent)
{
    static bool registered = []() {
        qRegisterMetaType<Gui::LinuxCAD::Provider::Response>("Gui::LinuxCAD::Provider::Response");
        return true;
    }();
    Q_UNUSED(registered);
}

Provider::~Provider() = default;

Provider* Provider::createFromSettings(QObject* parent)
{
    QSettings s;
    const QString kind = s.value(QString::fromLatin1(kSettingProvider),
                                 QStringLiteral("openai-compatible")).toString();
    if (kind.compare(QStringLiteral("anthropic"), Qt::CaseInsensitive) == 0) {
        return new AnthropicProvider(parent);
    }
    // Default: OpenAI-compatible. Endpoint defaults to OpenAI's API but the
    // user can point it at Ollama / OpenRouter / LM Studio etc.
    return new OpenAIProvider(parent);
}

} // namespace LinuxCAD
} // namespace Gui
