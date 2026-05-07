// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QJsonObject>
#include <QTimer>
#endif

#include "MockProvider.h"

namespace Gui {
namespace LinuxCAD {

MockProvider::MockProvider(QObject* parent)
    : Provider(parent)
{
}

MockProvider::~MockProvider()
{
    cancel();
}

QString MockProvider::displayName() const
{
    return QStringLiteral("Mock");
}

bool MockProvider::isConfigured() const
{
    return true;
}

bool MockProvider::inFlight() const
{
    return timer_ != nullptr && timer_->isActive();
}

void MockProvider::cancel()
{
    if (timer_ != nullptr) {
        timer_->stop();
        timer_->deleteLater();
        timer_ = nullptr;
    }
    inFlight_ = false;
    Q_EMIT stateChanged();
}

void MockProvider::complete(const std::vector<Message>& /*messages*/,
                              const QJsonArray& /*toolSchemas*/)
{
    cancel();

    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    connect(timer_, &QTimer::timeout, this, &MockProvider::onTimerFired);
    inFlight_ = true;
    timer_->start(250);
    Q_EMIT stateChanged();
}

void MockProvider::onTimerFired()
{
    inFlight_ = false;
    if (timer_ != nullptr) {
        timer_->deleteLater();
        timer_ = nullptr;
    }

    Response r;
    r.ok = true;
    r.text = QStringLiteral(
        "(mock) Suggested next step: start a sketch on XY for E2E AI testing.");

    ToolCall tc;
    tc.id = QStringLiteral("call_mock_new_sketch");
    tc.name = QStringLiteral("new_sketch");
    QJsonObject args;
    args.insert(QStringLiteral("plane"), QStringLiteral("XY"));
    tc.arguments = args;
    r.toolCalls.push_back(tc);

    Q_EMIT responded(r);
    Q_EMIT stateChanged();
}

} // namespace LinuxCAD
} // namespace Gui
