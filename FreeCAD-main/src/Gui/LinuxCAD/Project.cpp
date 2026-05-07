// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QString>
#include <QTextStream>
#endif

#include "Project.h"

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr int kSchemaVersion = 1;

QString toForward(const QString& path)
{
    QString p = path;
    p.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return p;
}

} // namespace

QString ProjectMember::kindToString(ProjectMember::Kind k)
{
    switch (k) {
        case Kind::Part:      return QStringLiteral("part");
        case Kind::Assembly:  return QStringLiteral("assembly");
        case Kind::Drawing:   return QStringLiteral("drawing");
        case Kind::Reference: return QStringLiteral("reference");
        case Kind::Asset:     return QStringLiteral("asset");
        case Kind::Unknown:   break;
    }
    return QStringLiteral("unknown");
}

ProjectMember::Kind ProjectMember::kindFromString(const QString& s)
{
    const QString v = s.toLower();
    if (v == QLatin1String("part"))      return Kind::Part;
    if (v == QLatin1String("assembly"))  return Kind::Assembly;
    if (v == QLatin1String("drawing"))   return Kind::Drawing;
    if (v == QLatin1String("reference")) return Kind::Reference;
    if (v == QLatin1String("asset"))     return Kind::Asset;
    return Kind::Unknown;
}

ProjectMember::Kind ProjectMember::inferKindFromPath(const QString& path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("fcstd"))               return Kind::Part;
    if (suffix == QLatin1String("step") || suffix == QLatin1String("stp")) return Kind::Reference;
    if (suffix == QLatin1String("iges") || suffix == QLatin1String("igs")) return Kind::Reference;
    if (suffix == QLatin1String("stl"))                 return Kind::Reference;
    if (suffix == QLatin1String("obj"))                 return Kind::Reference;
    if (suffix == QLatin1String("brep") || suffix == QLatin1String("brp")) return Kind::Reference;
    if (suffix == QLatin1String("dxf"))                 return Kind::Drawing;
    if (suffix == QLatin1String("svg"))                 return Kind::Drawing;
    if (suffix == QLatin1String("png") || suffix == QLatin1String("jpg")
     || suffix == QLatin1String("jpeg") || suffix == QLatin1String("pdf")) return Kind::Asset;
    return Kind::Unknown;
}

QString Project::rootDir() const
{
    if (filePath_.isEmpty()) {
        return QString();
    }
    return QFileInfo(filePath_).absolutePath();
}

QString Project::absolutePathFor(const ProjectMember& m) const
{
    if (m.relativePath.isEmpty()) {
        return QString();
    }
    if (QDir::isAbsolutePath(m.relativePath)) {
        return QDir::cleanPath(m.relativePath);
    }
    return QDir::cleanPath(QDir(rootDir()).absoluteFilePath(m.relativePath));
}

Project Project::newEmpty(const QString& projectName, const QString& filePath)
{
    Project p;
    p.filePath_   = filePath;
    p.name_       = projectName;
    p.createdAt_  = QDateTime::currentDateTime();
    p.modifiedAt_ = p.createdAt_;
    return p;
}

bool Project::load(const QString& path, QString* errorOut)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut) {
            *errorOut = QStringLiteral("Cannot open file: %1").arg(file.errorString());
        }
        return false;
    }

    QJsonParseError perr{};
    const QByteArray bytes = file.readAll();
    file.close();
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Malformed project file: %1").arg(perr.errorString());
        }
        return false;
    }

    const QJsonObject root = doc.object();
    filePath_     = path;
    name_         = root.value(QStringLiteral("name")).toString();
    description_  = root.value(QStringLiteral("description")).toString();
    createdAt_    = QDateTime::fromString(root.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    modifiedAt_   = QDateTime::fromString(root.value(QStringLiteral("modifiedAt")).toString(), Qt::ISODate);
    if (!createdAt_.isValid())  createdAt_  = QDateTime::currentDateTime();
    if (!modifiedAt_.isValid()) modifiedAt_ = QDateTime::currentDateTime();

    members_.clear();
    const QJsonArray arr = root.value(QStringLiteral("members")).toArray();
    for (const auto& v : arr) {
        const QJsonObject m = v.toObject();
        ProjectMember pm;
        pm.kind         = ProjectMember::kindFromString(m.value(QStringLiteral("kind")).toString());
        pm.name         = m.value(QStringLiteral("name")).toString();
        pm.relativePath = toForward(m.value(QStringLiteral("path")).toString());
        pm.notes        = m.value(QStringLiteral("notes")).toString();
        if (pm.relativePath.isEmpty()) {
            continue;
        }
        if (pm.name.isEmpty()) {
            pm.name = QFileInfo(pm.relativePath).fileName();
        }
        if (pm.kind == ProjectMember::Kind::Unknown) {
            pm.kind = ProjectMember::inferKindFromPath(pm.relativePath);
        }
        members_.append(pm);
    }
    return true;
}

bool Project::save(QString* errorOut)
{
    if (filePath_.isEmpty()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Project has no file path");
        }
        return false;
    }

    QJsonObject root;
    root[QStringLiteral("schemaVersion")] = kSchemaVersion;
    root[QStringLiteral("name")]          = name_;
    root[QStringLiteral("description")]   = description_;
    root[QStringLiteral("createdAt")]     = createdAt_.toString(Qt::ISODate);
    modifiedAt_                           = QDateTime::currentDateTime();
    root[QStringLiteral("modifiedAt")]    = modifiedAt_.toString(Qt::ISODate);

    QJsonArray arr;
    for (const auto& m : members_) {
        QJsonObject obj;
        obj[QStringLiteral("kind")]  = ProjectMember::kindToString(m.kind);
        obj[QStringLiteral("name")]  = m.name;
        obj[QStringLiteral("path")]  = toForward(m.relativePath);
        if (!m.notes.isEmpty()) {
            obj[QStringLiteral("notes")] = m.notes;
        }
        arr.append(obj);
    }
    root[QStringLiteral("members")] = arr;

    QSaveFile out(filePath_);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut) {
            *errorOut = QStringLiteral("Cannot open project for writing: %1").arg(out.errorString());
        }
        return false;
    }
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (out.write(bytes) != bytes.size()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Short write: %1").arg(out.errorString());
        }
        out.cancelWriting();
        return false;
    }
    if (!out.commit()) {
        if (errorOut) {
            *errorOut = QStringLiteral("Commit failed: %1").arg(out.errorString());
        }
        return false;
    }
    return true;
}

} // namespace LinuxCAD
} // namespace Gui
