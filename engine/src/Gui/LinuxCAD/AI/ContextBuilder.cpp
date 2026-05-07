// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Workbench.h>
#include <Gui/WorkbenchManager.h>

#include "ContextBuilder.h"
#include "Provider.h"

namespace Gui {
namespace LinuxCAD {

namespace {

bool redactNames()
{
    QSettings s;
    return s.value(QString::fromLatin1(Provider::kSettingRedactNames), false).toBool();
}

QString safeName(const std::string& name)
{
    if (!redactNames()) {
        return QString::fromStdString(name);
    }
    QByteArray h = QCryptographicHash::hash(QByteArray::fromStdString(name),
                                            QCryptographicHash::Md5);
    return QStringLiteral("obj_") + QString::fromLatin1(h.toHex().left(8));
}

QJsonObject summarizeObject(App::DocumentObject* obj)
{
    QJsonObject out;
    if (obj == nullptr) {
        return out;
    }
    out.insert(QStringLiteral("name"), safeName(obj->getNameInDocument() ? obj->getNameInDocument() : ""));
    out.insert(QStringLiteral("type"), QString::fromUtf8(obj->getTypeId().getName()));
    const std::string label = obj->Label.getStrValue();
    if (!label.empty()) {
        out.insert(QStringLiteral("label"), redactNames()
                                                ? QStringLiteral("(redacted)")
                                                : QString::fromStdString(label));
    }
    return out;
}

QJsonArray summarizeAllObjects(App::Document* doc, int limit = 24)
{
    QJsonArray arr;
    if (doc == nullptr) {
        return arr;
    }
    auto objs = doc->getObjects();
    int count = 0;
    for (auto* o : objs) {
        if (count >= limit) {
            break;
        }
        arr.append(summarizeObject(o));
        ++count;
    }
    return arr;
}

QJsonArray summarizeSelection()
{
    QJsonArray arr;
    auto sel = Gui::Selection().getCompleteSelection();
    int count = 0;
    for (const auto& s : sel) {
        if (count >= 16) {
            break;
        }
        QJsonObject obj;
        obj.insert(QStringLiteral("doc"), redactNames()
                                              ? QStringLiteral("(redacted)")
                                              : QString::fromStdString(s.DocName ? s.DocName : ""));
        obj.insert(QStringLiteral("object"),
                   safeName(s.FeatName ? s.FeatName : ""));
        if (s.SubName != nullptr && s.SubName[0] != '\0') {
            obj.insert(QStringLiteral("sub"),
                       redactNames() ? QStringLiteral("(sub)")
                                     : QString::fromUtf8(s.SubName));
        }
        if (s.TypeName != nullptr && s.TypeName[0] != '\0') {
            obj.insert(QStringLiteral("type"), QString::fromUtf8(s.TypeName));
        }
        arr.append(obj);
        ++count;
    }
    return arr;
}

QString activeWorkbenchName()
{
    if (auto* mgr = Gui::WorkbenchManager::instance()) {
        return QString::fromStdString(mgr->activeName());
    }
    return QString();
}

} // namespace

bool ContextBuilder::perDocumentOptOut()
{
    auto* doc = App::GetApplication().getActiveDocument();
    if (doc == nullptr) {
        return false;
    }
    QSettings s;
    const QStringList opts =
        s.value(QString::fromLatin1(Provider::kSettingPerDocOptOut)).toStringList();
    return opts.contains(QString::fromUtf8(doc->getName()));
}

QJsonObject ContextBuilder::build()
{
    QJsonObject ctx;
    ctx.insert(QStringLiteral("schemaVersion"), 1);

    auto* doc = App::GetApplication().getActiveDocument();
    if (doc == nullptr) {
        ctx.insert(QStringLiteral("docOpen"), false);
        return ctx;
    }

    ctx.insert(QStringLiteral("docOpen"), true);
    ctx.insert(QStringLiteral("doc"), redactNames()
                                          ? QStringLiteral("(redacted)")
                                          : QString::fromUtf8(doc->getName()));
    ctx.insert(QStringLiteral("workbench"), activeWorkbenchName());
    ctx.insert(QStringLiteral("objects"),   summarizeAllObjects(doc));
    ctx.insert(QStringLiteral("selection"), summarizeSelection());

    // Recent operations from undo stack (names only, capped at 8).
    QJsonArray recent;
    auto names = doc->getAvailableUndoNames();
    int taken = 0;
    for (auto it = names.rbegin(); it != names.rend() && taken < 8; ++it, ++taken) {
        recent.append(QString::fromStdString(*it));
    }
    ctx.insert(QStringLiteral("recentOps"), recent);

    return ctx;
}

} // namespace LinuxCAD
} // namespace Gui
