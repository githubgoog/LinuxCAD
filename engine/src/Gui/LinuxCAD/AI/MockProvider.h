// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_MOCK_PROVIDER_H
#define GUI_LINUXCAD_AI_MOCK_PROVIDER_H

#include <FCGlobal.h>

#include "Provider.h"

class QTimer;

namespace Gui {
namespace LinuxCAD {

/// Local test provider that returns a canned tool call after a short delay.
class GuiExport MockProvider : public Provider
{
    Q_OBJECT

public:
    explicit MockProvider(QObject* parent = nullptr);
    ~MockProvider() override;

    QString displayName() const override;
    bool    isConfigured() const override;
    void    complete(const std::vector<Message>& messages,
                     const QJsonArray& toolSchemas) override;
    void    cancel() override;
    bool    inFlight() const override;

private Q_SLOTS:
    void onTimerFired();

private:
    QTimer* timer_      = nullptr;
    bool    inFlight_   = false;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_MOCK_PROVIDER_H
