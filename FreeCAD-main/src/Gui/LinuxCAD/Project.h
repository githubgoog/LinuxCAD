// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_PROJECT_H
#define GUI_LINUXCAD_PROJECT_H

#include <FCGlobal.h>
#include <QDateTime>
#include <QList>
#include <QString>

namespace Gui {
namespace LinuxCAD {

/// One member of a LinuxCAD project (a part, a drawing, an assembly, etc.).
struct GuiExport ProjectMember
{
    enum class Kind {
        Part,        ///< .FCStd that contains modeling bodies
        Assembly,    ///< .FCStd that uses the Assembly workbench
        Drawing,     ///< TechDraw .FCStd
        Reference,   ///< External STEP/IGES/STL/etc.
        Asset,       ///< Anything else (PDF, image, notes)
        Unknown,
    };

    Kind    kind = Kind::Unknown;
    QString name;        ///< Human-readable display name
    QString relativePath;///< Path relative to project file (forward slashes)
    QString notes;

    static QString kindToString(Kind k);
    static Kind    kindFromString(const QString& s);
    static Kind    inferKindFromPath(const QString& path);
};

/// A LinuxCAD project, persisted as a JSON file (`.lcadproj`).
///
/// A project groups several FreeCAD documents and external assets into a
/// single coherent unit, layered above FreeCAD's own per-document model.
class GuiExport Project
{
public:
    Project() = default;

    /// Project file path on disk (the .lcadproj file). Empty if unsaved.
    QString filePath() const { return filePath_; }
    void setFilePath(const QString& path) { filePath_ = path; }

    /// Project root directory (the directory that contains the .lcadproj file).
    QString rootDir() const;

    QString name() const { return name_; }
    void    setName(const QString& v) { name_ = v; }

    QString description() const { return description_; }
    void    setDescription(const QString& v) { description_ = v; }

    QDateTime createdAt() const { return createdAt_; }
    QDateTime modifiedAt() const { return modifiedAt_; }

    const QList<ProjectMember>& members() const { return members_; }
    QList<ProjectMember>&       members()       { return members_; }

    /// Resolve a member's absolute path.
    QString absolutePathFor(const ProjectMember& m) const;

    /// Read a project from a file. Returns false on any IO/parse error,
    /// and writes a human-readable explanation into `errorOut` (if non-null).
    bool load(const QString& path, QString* errorOut = nullptr);

    /// Persist this project to its current `filePath()`. Returns false on error.
    bool save(QString* errorOut = nullptr);

    /// Build a new empty project with sensible defaults.
    static Project newEmpty(const QString& projectName, const QString& filePath);

private:
    QString              filePath_;
    QString              name_;
    QString              description_;
    QDateTime            createdAt_   = QDateTime::currentDateTime();
    QDateTime            modifiedAt_  = QDateTime::currentDateTime();
    QList<ProjectMember> members_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_PROJECT_H
