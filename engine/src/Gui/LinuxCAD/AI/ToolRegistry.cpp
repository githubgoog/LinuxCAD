// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Selection/Selection.h>

#include "ToolRegistry.h"

namespace Gui {
namespace LinuxCAD {

namespace {

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

bool runFreeCADCommand(const char* name)
{
    auto* app = Gui::Application::Instance;
    if (app == nullptr) {
        return false;
    }
    auto* cmd = app->commandManager().getCommandByName(name);
    if (cmd == nullptr) {
        return false;
    }
    cmd->invoke(0);
    return true;
}

bool selectByPath(const QString& docName, const QString& objectName, const QString& subName)
{
    if (docName.isEmpty() || objectName.isEmpty()) {
        return false;
    }
    Gui::Selection().clearSelection();
    return Gui::Selection().addSelection(docName.toUtf8().constData(),
                                          objectName.toUtf8().constData(),
                                          subName.isEmpty() ? nullptr
                                                            : subName.toUtf8().constData());
}

App::Document* activeDoc()
{
    return App::GetApplication().getActiveDocument();
}

QJsonObject schemaObject(const QString& description, const QJsonObject& props,
                         const QStringList& required = {})
{
    QJsonObject schema;
    schema.insert(QStringLiteral("type"), QStringLiteral("object"));
    schema.insert(QStringLiteral("properties"), props);
    if (!required.isEmpty()) {
        QJsonArray req;
        for (const auto& r : required) req.append(r);
        schema.insert(QStringLiteral("required"), req);
    }
    Q_UNUSED(description);
    return schema;
}

QJsonObject numProp(const QString& description, double minVal = 0.0)
{
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("number"));
    o.insert(QStringLiteral("description"), description);
    o.insert(QStringLiteral("minimum"), minVal);
    return o;
}

QJsonObject strProp(const QString& description)
{
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("string"));
    o.insert(QStringLiteral("description"), description);
    return o;
}

ToolRegistry::ExecResult invokeWithSelection(const QJsonObject& args,
                                             const char* freecadCommand,
                                             const QString& humanName)
{
    ToolRegistry::ExecResult r;
    auto* doc = activeDoc();
    if (doc == nullptr) {
        r.ok = false;
        r.message = QStringLiteral("No active document");
        return r;
    }

    const QString docName    = args.value(QStringLiteral("document"))
                                  .toString(QString::fromUtf8(doc->getName()));
    const QString objectName = args.value(QStringLiteral("object")).toString();
    const QString subName    = args.value(QStringLiteral("sub")).toString();

    if (objectName.isEmpty()) {
        r.ok = false;
        r.message = QStringLiteral("Missing 'object' parameter");
        return r;
    }

    if (!selectByPath(docName, objectName, subName)) {
        r.ok = false;
        r.message = QStringLiteral("Could not select '%1' / '%2'").arg(objectName, subName);
        return r;
    }

    const std::string txnName = humanName.toStdString();
    doc->openTransaction(txnName.c_str());
    try {
        if (!runFreeCADCommand(freecadCommand)) {
            doc->abortTransaction();
            r.ok = false;
            r.message = QStringLiteral("FreeCAD command '%1' not available")
                            .arg(QString::fromLatin1(freecadCommand));
            return r;
        }
        doc->commitTransaction();
    }
    catch (...) {
        doc->abortTransaction();
        r.ok = false;
        r.message = QStringLiteral("Exception while running '%1'")
                        .arg(QString::fromLatin1(freecadCommand));
        return r;
    }

    r.ok = true;
    r.humanizedSummary = humanName;
    r.message = QStringLiteral("Done.");
    return r;
}

} // namespace

ToolRegistry& ToolRegistry::instance()
{
    static ToolRegistry inst;
    return inst;
}

ToolRegistry::ToolRegistry()
{
    registerDefaults();
}

void ToolRegistry::addTool(ToolInfo info)
{
    tools_.push_back(std::move(info));
}

const ToolRegistry::ToolInfo* ToolRegistry::find(const QString& name) const
{
    for (const auto& t : tools_) {
        if (t.name == name) {
            return &t;
        }
    }
    return nullptr;
}

ToolRegistry::ExecResult ToolRegistry::execute(const QString& name,
                                               const QJsonObject& args) const
{
    if (auto* t = find(name)) {
        return t->execute(args);
    }
    ExecResult r;
    r.ok = false;
    r.message = QStringLiteral("Unknown tool '%1'").arg(name);
    return r;
}

QJsonArray ToolRegistry::openAIToolSchemas() const
{
    QJsonArray arr;
    for (const auto& t : tools_) {
        QJsonObject fn;
        fn.insert(QStringLiteral("name"), t.name);
        fn.insert(QStringLiteral("description"), t.description);
        fn.insert(QStringLiteral("parameters"), t.parametersSchema);
        QJsonObject wrap;
        wrap.insert(QStringLiteral("type"), QStringLiteral("function"));
        wrap.insert(QStringLiteral("function"), fn);
        arr.append(wrap);
    }
    return arr;
}

void ToolRegistry::registerDefaults()
{
    // ---- new_sketch ------------------------------------------------------
    {
        ToolInfo t;
        t.name = QStringLiteral("new_sketch");
        t.description = QStringLiteral(
            "Start a new sketch on a base plane (XY/XZ/YZ) inside the active body.");
        QJsonObject props;
        QJsonObject planeProp;
        planeProp.insert(QStringLiteral("type"), QStringLiteral("string"));
        planeProp.insert(QStringLiteral("enum"), QJsonArray{QStringLiteral("XY"),
                                                            QStringLiteral("XZ"),
                                                            QStringLiteral("YZ")});
        props.insert(QStringLiteral("plane"), planeProp);
        t.parametersSchema = schemaObject(t.description, props, {QStringLiteral("plane")});

        t.execute = [](const QJsonObject& /*args*/) -> ExecResult {
            ExecResult r;
            auto* doc = activeDoc();
            if (doc == nullptr) {
                r.message = QStringLiteral("No active document");
                return r;
            }
            doc->openTransaction("AI: New Sketch");
            try {
                runFreeCADCommand("PartDesign_Body");
                runFreeCADCommand("Sketcher_NewSketch");
                doc->commitTransaction();
                r.ok = true;
                r.humanizedSummary = QStringLiteral("Started a new sketch");
            }
            catch (...) {
                doc->abortTransaction();
                r.message = QStringLiteral("Could not start a sketch");
            }
            return r;
        };
        addTool(std::move(t));
    }

    // ---- add_fillet ------------------------------------------------------
    {
        ToolInfo t;
        t.name = QStringLiteral("add_fillet");
        t.description = QStringLiteral("Add a fillet to a selected edge with the given radius.");
        QJsonObject props;
        props.insert(QStringLiteral("object"), strProp(QStringLiteral("Feature object name")));
        props.insert(QStringLiteral("sub"),    strProp(QStringLiteral("Sub-element name (Edge1, ...)")));
        props.insert(QStringLiteral("radius"), numProp(QStringLiteral("Fillet radius (mm)")));
        t.parametersSchema = schemaObject(t.description, props,
                                          {QStringLiteral("object"), QStringLiteral("radius")});
        t.execute = [](const QJsonObject& args) {
            return invokeWithSelection(args, "PartDesign_Fillet",
                                        QStringLiteral("AI: Fillet"));
        };
        addTool(std::move(t));
    }

    // ---- add_chamfer -----------------------------------------------------
    {
        ToolInfo t;
        t.name = QStringLiteral("add_chamfer");
        t.description = QStringLiteral("Add a chamfer to a selected edge.");
        QJsonObject props;
        props.insert(QStringLiteral("object"), strProp(QStringLiteral("Feature object name")));
        props.insert(QStringLiteral("sub"),    strProp(QStringLiteral("Sub-element name")));
        props.insert(QStringLiteral("size"),   numProp(QStringLiteral("Chamfer size (mm)")));
        t.parametersSchema = schemaObject(t.description, props,
                                          {QStringLiteral("object"), QStringLiteral("size")});
        t.execute = [](const QJsonObject& args) {
            return invokeWithSelection(args, "PartDesign_Chamfer",
                                        QStringLiteral("AI: Chamfer"));
        };
        addTool(std::move(t));
    }

    // ---- add_pad ---------------------------------------------------------
    {
        ToolInfo t;
        t.name = QStringLiteral("add_pad");
        t.description = QStringLiteral("Pad / extrude the selected sketch by a length.");
        QJsonObject props;
        props.insert(QStringLiteral("object"), strProp(QStringLiteral("Sketch object name")));
        props.insert(QStringLiteral("length"), numProp(QStringLiteral("Pad length (mm)")));
        t.parametersSchema = schemaObject(t.description, props,
                                          {QStringLiteral("object"), QStringLiteral("length")});
        t.execute = [](const QJsonObject& args) {
            return invokeWithSelection(args, "PartDesign_Pad",
                                        QStringLiteral("AI: Pad"));
        };
        addTool(std::move(t));
    }

    // ---- add_pocket ------------------------------------------------------
    {
        ToolInfo t;
        t.name = QStringLiteral("add_pocket");
        t.description = QStringLiteral("Pocket the selected sketch into the body.");
        QJsonObject props;
        props.insert(QStringLiteral("object"), strProp(QStringLiteral("Sketch object name")));
        props.insert(QStringLiteral("depth"),  numProp(QStringLiteral("Pocket depth (mm)")));
        t.parametersSchema = schemaObject(t.description, props,
                                          {QStringLiteral("object"), QStringLiteral("depth")});
        t.execute = [](const QJsonObject& args) {
            return invokeWithSelection(args, "PartDesign_Pocket",
                                        QStringLiteral("AI: Pocket"));
        };
        addTool(std::move(t));
    }

    // ---- linear_pattern --------------------------------------------------
    {
        ToolInfo t;
        t.name = QStringLiteral("linear_pattern");
        t.description = QStringLiteral("Repeat the selected feature in a linear pattern.");
        QJsonObject props;
        props.insert(QStringLiteral("object"), strProp(QStringLiteral("Feature to repeat")));
        props.insert(QStringLiteral("count"),  numProp(QStringLiteral("Total instances"), 2));
        props.insert(QStringLiteral("spacing"),numProp(QStringLiteral("Spacing (mm)")));
        t.parametersSchema = schemaObject(t.description, props,
                                          {QStringLiteral("object"), QStringLiteral("count"),
                                           QStringLiteral("spacing")});
        t.execute = [](const QJsonObject& args) {
            return invokeWithSelection(args, "PartDesign_LinearPattern",
                                        QStringLiteral("AI: Linear pattern"));
        };
        addTool(std::move(t));
    }

    // ---- polar_pattern --------------------------------------------------
    {
        ToolInfo t;
        t.name = QStringLiteral("polar_pattern");
        t.description = QStringLiteral("Repeat the selected feature around an axis.");
        QJsonObject props;
        props.insert(QStringLiteral("object"), strProp(QStringLiteral("Feature to repeat")));
        props.insert(QStringLiteral("count"),  numProp(QStringLiteral("Total instances"), 2));
        props.insert(QStringLiteral("angle"),  numProp(QStringLiteral("Sweep angle (deg)")));
        t.parametersSchema = schemaObject(t.description, props,
                                          {QStringLiteral("object"), QStringLiteral("count")});
        t.execute = [](const QJsonObject& args) {
            return invokeWithSelection(args, "PartDesign_PolarPattern",
                                        QStringLiteral("AI: Polar pattern"));
        };
        addTool(std::move(t));
    }

    // ---- mirror_feature -------------------------------------------------
    {
        ToolInfo t;
        t.name = QStringLiteral("mirror_feature");
        t.description = QStringLiteral("Mirror the selected feature across a plane.");
        QJsonObject props;
        props.insert(QStringLiteral("object"), strProp(QStringLiteral("Feature to mirror")));
        props.insert(QStringLiteral("plane"),  strProp(QStringLiteral("Mirror plane (XY/XZ/YZ)")));
        t.parametersSchema = schemaObject(t.description, props, {QStringLiteral("object")});
        t.execute = [](const QJsonObject& args) {
            return invokeWithSelection(args, "PartDesign_Mirrored",
                                        QStringLiteral("AI: Mirror"));
        };
        addTool(std::move(t));
    }
}

} // namespace LinuxCAD
} // namespace Gui
