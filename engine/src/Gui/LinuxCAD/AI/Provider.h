// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_PROVIDER_H
#define GUI_LINUXCAD_AI_PROVIDER_H

#include <FCGlobal.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <vector>

namespace Gui {
namespace LinuxCAD {

/// Abstract AI provider used by SuggestionEngine, ChatSidebar (later), and
/// text-to-sketch (later). Each concrete provider talks to a remote endpoint
/// (OpenAI, Anthropic, OpenAI-compatible URLs like Ollama / OpenRouter / LM
/// Studio) over HTTPS via Qt's network stack.
///
/// Providers are stateless across requests; only the active request lives in
/// the object. The result is delivered asynchronously via Qt signals.
class GuiExport Provider : public QObject
{
    Q_OBJECT

public:
    /// One message in a chat-style conversation.
    struct Message
    {
        QString role;     ///< "system" / "user" / "assistant" / "tool"
        QString content;  ///< plain text
    };

    /// One tool call returned by the provider.
    struct ToolCall
    {
        QString     id;
        QString     name;
        QJsonObject arguments;
    };

    /// Final response for one completion. We currently don't stream chunks
    /// back to the UI - SuggestionEngine just waits for the final response -
    /// but the API is shaped so streaming can be wired in later.
    struct Response
    {
        QString               text;
        std::vector<ToolCall> toolCalls;
        bool                  ok = false;
        QString               errorMessage;
    };

    explicit Provider(QObject* parent = nullptr);
    ~Provider() override;

    /// Provider type / display name (e.g. "OpenAI", "Anthropic", "Ollama").
    virtual QString displayName() const = 0;

    /// True if a key + endpoint are configured well enough to attempt a call.
    virtual bool isConfigured() const = 0;

    /// Fire one completion. The response is delivered via the `responded`
    /// signal. The caller should call `cancel()` if it loses interest before
    /// the response arrives.
    virtual void complete(const std::vector<Message>& messages,
                          const QJsonArray& toolSchemas) = 0;

    /// Cancel any in-flight request. No-op if there's nothing to cancel.
    virtual void cancel() = 0;

    /// Returns true if a request is currently in flight.
    virtual bool inFlight() const = 0;

    // ---------------------------------------------------------------- Settings

    /// Setting keys (under QSettings) that providers honor.
    static constexpr const char* kSettingProvider     = "LinuxCAD/AI/Provider";
    static constexpr const char* kSettingEndpoint     = "LinuxCAD/AI/Endpoint";
    static constexpr const char* kSettingModel        = "LinuxCAD/AI/Model";
    static constexpr const char* kSettingApiKey       = "LinuxCAD/AI/ApiKey";
    static constexpr const char* kSettingEnabled      = "LinuxCAD/AI/Enabled";
    static constexpr const char* kSettingConsentGiven = "LinuxCAD/AI/ConsentGivenV1";
    static constexpr const char* kSettingMaxRequests  = "LinuxCAD/AI/MaxRequestsPerSession";
    static constexpr const char* kSettingPerDocOptOut = "LinuxCAD/AI/PerDocOptOut";
    static constexpr const char* kSettingRedactNames  = "LinuxCAD/AI/RedactNames";

    /// Construct a provider based on the user's QSettings choice. Returns a
    /// disabled stub if nothing is configured. Caller takes ownership through
    /// QObject parenting.
    static Provider* createFromSettings(QObject* parent);

Q_SIGNALS:
    /// Emitted when an in-flight request finishes (success or error).
    void responded(const Provider::Response& response);

    /// Emitted when the provider transitions between states. SuggestionEngine
    /// uses this to update the AI status badge in the TopBar.
    void stateChanged();
};

} // namespace LinuxCAD
} // namespace Gui

Q_DECLARE_METATYPE(Gui::LinuxCAD::Provider::Response)

#endif // GUI_LINUXCAD_AI_PROVIDER_H
