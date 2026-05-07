// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_SUGGESTION_ENGINE_H
#define GUI_LINUXCAD_AI_SUGGESTION_ENGINE_H

#include <FCGlobal.h>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QString>

class QTimer;

namespace Gui {
namespace LinuxCAD {

class Provider;
class GhostToast;

/// Debounced listener that watches FreeCAD's selection / document signals
/// and, when activity quiets down, asks the configured Provider for one
/// Action JSON. The action is rendered as a `GhostToast` for the user to
/// preview / accept / dismiss; nothing mutates the document until they
/// hit Tab.
class GuiExport SuggestionEngine : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Disabled,   ///< AI is off / unconfigured
        Idle,       ///< No request in flight, waiting on user activity
        Thinking,   ///< Request in flight
        Cooldown,   ///< Just delivered a suggestion, brief cool-off
        Error,      ///< Last request failed
    };
    Q_ENUM(State)

    SuggestionEngine(Provider* provider, GhostToast* toast, QObject* parent = nullptr);
    ~SuggestionEngine() override;

    /// Begin observing FreeCAD signals. Idempotent.
    void start();

    /// Stop observing and cancel any in-flight request.
    void stop();

    State state() const { return state_; }

    /// Total requests sent during this app session (for rate limit checks).
    int requestsThisSession() const { return requestsThisSession_; }

Q_SIGNALS:
    void stateChanged(State);

private Q_SLOTS:
    void onMaybeFire();
    void onProviderResponded(const Gui::LinuxCAD::Provider::Response&);
    void onProviderStateChanged();

private:
    void scheduleRefresh();
    void setState(State s);
    bool sessionAllowsAnotherRequest() const;
    void buildAndSendPrompt();

    QPointer<Provider>   provider_;
    QPointer<GhostToast> toast_;
    QTimer*              debounce_       = nullptr;
    QTimer*              cooldown_       = nullptr;
    State                state_          = State::Disabled;
    int                  requestsThisSession_ = 0;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_SUGGESTION_ENGINE_H
