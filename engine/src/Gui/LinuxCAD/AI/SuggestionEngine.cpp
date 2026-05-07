// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QString>
#include <QTimer>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Selection/Selection.h>

#include "ContextBuilder.h"
#include "GhostToast.h"
#include "Provider.h"
#include "SuggestionEngine.h"
#include "ToolRegistry.h"

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr int kDebounceMs       = 1500;
constexpr int kCooldownMs       = 4000;
constexpr int kDefaultMaxPerSession = 80;

QString systemPrompt()
{
    return QStringLiteral(
        "You are an inline modeling copilot embedded in LinuxCAD, a CAD app "
        "built on FreeCAD. Look at the user's current modeling context and "
        "suggest at most one helpful next operation by invoking exactly one "
        "tool. Prefer small, well-scoped suggestions. If nothing useful is "
        "obvious, return an empty assistant message and no tool calls. Never "
        "invent objects or measurements; only act on objects in the context. "
        "Always include a 1-line explanation in your text content of why "
        "the action helps.");
}

} // namespace

SuggestionEngine::SuggestionEngine(Provider* provider, GhostToast* toast, QObject* parent)
    : QObject(parent)
    , provider_(provider)
    , toast_(toast)
    , debounce_(new QTimer(this))
    , cooldown_(new QTimer(this))
{
    debounce_->setSingleShot(true);
    debounce_->setInterval(kDebounceMs);
    cooldown_->setSingleShot(true);
    cooldown_->setInterval(kCooldownMs);

    connect(debounce_, &QTimer::timeout, this, &SuggestionEngine::onMaybeFire);
    connect(cooldown_, &QTimer::timeout, this, [this]() { setState(State::Idle); });

    if (provider_) {
        connect(provider_.data(), &Provider::responded,
                this, &SuggestionEngine::onProviderResponded);
        connect(provider_.data(), &Provider::stateChanged,
                this, &SuggestionEngine::onProviderStateChanged);
    }
}

SuggestionEngine::~SuggestionEngine()
{
    stop();
}

void SuggestionEngine::setState(State s)
{
    if (state_ != s) {
        state_ = s;
        Q_EMIT stateChanged(s);
    }
}

void SuggestionEngine::start()
{
    if (provider_ && provider_->isConfigured()) {
        setState(State::Idle);
    }
    else {
        setState(State::Disabled);
    }

    // Watch selection changes via FreeCAD's Subject<SelectionChanges> would
    // require Subject hooking. The simpler approach: poll selection on a
    // QTimer that ticks the debounce when changes are detected. Selection
    // changes are usually paired with a click, which fires a Qt event - so
    // we hook off the Selection() singleton's signal-like behavior by just
    // scheduling on every focus change.
    //
    // For MVP we trigger on any document change via App::Application's
    // signalChangedDocument and signalNewDocument.
    try {
        App::GetApplication().signalNewDocument.connect(
            [this](const App::Document&, bool) { scheduleRefresh(); });
        App::GetApplication().signalChangedDocument.connect(
            [this](const App::Document&, const App::Property&) { scheduleRefresh(); });
        App::GetApplication().signalDeletedDocument.connect(
            [this]() { scheduleRefresh(); });
    }
    catch (...) {
        // Defensive: never let signal hookup break the UI.
    }
}

void SuggestionEngine::stop()
{
    if (provider_) {
        provider_->cancel();
    }
    if (debounce_) {
        debounce_->stop();
    }
    if (cooldown_) {
        cooldown_->stop();
    }
    setState(State::Disabled);
}

void SuggestionEngine::scheduleRefresh()
{
    if (state_ == State::Disabled) {
        return;
    }
    if (state_ == State::Cooldown) {
        return;  // wait out the cool-off
    }
    if (state_ == State::Thinking) {
        return;  // already busy
    }
    if (debounce_ != nullptr) {
        debounce_->start();
    }
}

bool SuggestionEngine::sessionAllowsAnotherRequest() const
{
    QSettings s;
    const int cap = s.value(QString::fromLatin1(Provider::kSettingMaxRequests),
                            kDefaultMaxPerSession).toInt();
    if (cap <= 0) {
        return true;
    }
    return requestsThisSession_ < cap;
}

void SuggestionEngine::onMaybeFire()
{
    if (state_ != State::Idle) {
        return;
    }
    if (!provider_ || !provider_->isConfigured()) {
        setState(State::Disabled);
        return;
    }
    if (ContextBuilder::perDocumentOptOut()) {
        return;
    }
    if (!sessionAllowsAnotherRequest()) {
        return;
    }
    buildAndSendPrompt();
}

void SuggestionEngine::buildAndSendPrompt()
{
    const QJsonObject ctx = ContextBuilder::build();
    if (!ctx.value(QStringLiteral("docOpen")).toBool()) {
        return;  // nothing to suggest in an empty session
    }

    std::vector<Provider::Message> msgs;
    msgs.push_back({QStringLiteral("system"), systemPrompt()});
    Provider::Message user;
    user.role = QStringLiteral("user");
    user.content = QStringLiteral(
        "Modeling context (JSON):\n```json\n%1\n```\n"
        "If you see a useful next action, call exactly one tool. If not, "
        "return an empty message with no tool calls.")
            .arg(QString::fromUtf8(QJsonDocument(ctx).toJson(QJsonDocument::Indented)));
    msgs.push_back(user);

    setState(State::Thinking);
    ++requestsThisSession_;
    provider_->complete(msgs, ToolRegistry::instance().openAIToolSchemas());
}

void SuggestionEngine::onProviderStateChanged()
{
    if (provider_ != nullptr && provider_->inFlight()) {
        setState(State::Thinking);
    }
}

void SuggestionEngine::onProviderResponded(const Provider::Response& r)
{
    if (!r.ok) {
        setState(State::Error);
        Base::Console().log("LinuxCAD AI: %s\n", r.errorMessage.toUtf8().constData());
        // Brief stay in Error before going back to Idle.
        QTimer::singleShot(2500, this, [this]() {
            if (state_ == State::Error) setState(State::Idle);
        });
        return;
    }

    if (r.toolCalls.empty()) {
        // Nothing actionable - quiet success.
        setState(State::Cooldown);
        cooldown_->start();
        return;
    }

    const auto& call = r.toolCalls.front();
    if (toast_ != nullptr) {
        QString reason = r.text.trimmed();
        if (reason.isEmpty()) {
            reason = QStringLiteral("Suggested next step");
        }
        toast_->present(call.name, call.arguments, reason);
    }

    setState(State::Cooldown);
    cooldown_->start();
}

} // namespace LinuxCAD
} // namespace Gui
