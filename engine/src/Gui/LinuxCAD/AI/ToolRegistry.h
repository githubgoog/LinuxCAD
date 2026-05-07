// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_AI_TOOL_REGISTRY_H
#define GUI_LINUXCAD_AI_TOOL_REGISTRY_H

#include <FCGlobal.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <functional>
#include <vector>

namespace Gui {
namespace LinuxCAD {

/// Registry of validated, transactional FreeCAD operations callable by AI.
///
/// Each tool wraps an existing FreeCAD command behind a small JSON schema.
/// Tools execute inside an `App::Document::openTransaction()` so a single
/// Ctrl+Z fully reverts the change. Validation rejects malformed args before
/// any state mutation.
///
/// MVP tool set:
///   add_fillet, add_chamfer, add_pad, add_pocket,
///   linear_pattern, polar_pattern, mirror_feature,
///   new_sketch, add_dimension, add_constraint
class GuiExport ToolRegistry
{
public:
    /// One executed-tool result.
    struct ExecResult
    {
        bool    ok = false;
        QString message;          ///< Human-readable explanation
        QString humanizedSummary; ///< e.g. "Added 2 mm fillet"
    };

    /// One registered tool.
    struct ToolInfo
    {
        QString     name;
        QString     description;
        QJsonObject parametersSchema;  ///< OpenAI-compatible JSON schema

        /// Validates and runs the tool. Must wrap mutations in an
        /// openTransaction()/commit/abort pair.
        std::function<ExecResult(const QJsonObject&)> execute;
    };

    /// Singleton accessor.
    static ToolRegistry& instance();

    const std::vector<ToolInfo>& tools() const { return tools_; }

    /// Build the JSON schema array passed to the AI provider.
    QJsonArray openAIToolSchemas() const;

    /// Look up a tool by name. Returns nullptr if not registered.
    const ToolInfo* find(const QString& name) const;

    /// Convenience: validate + execute, returning the structured result.
    ExecResult execute(const QString& name, const QJsonObject& args) const;

private:
    ToolRegistry();
    ~ToolRegistry() = default;
    ToolRegistry(const ToolRegistry&) = delete;
    ToolRegistry& operator=(const ToolRegistry&) = delete;

    void registerDefaults();
    void addTool(ToolInfo info);

    std::vector<ToolInfo> tools_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_AI_TOOL_REGISTRY_H
