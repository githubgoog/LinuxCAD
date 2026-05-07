// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_PROJECTMANAGER_H
#define GUI_LINUXCAD_PROJECTMANAGER_H

#include <FCGlobal.h>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

#include "Project.h"

class QWidget;

namespace Gui {
namespace LinuxCAD {

/// LinuxCAD project manager service.
///
/// Owns the *single currently active project* (if any), tracks
/// recent projects via QSettings, and orchestrates open/save/close
/// flows including showing dialogs and opening member documents in
/// FreeCAD.
class GuiExport ProjectManager : public QObject
{
    Q_OBJECT

public:
    explicit ProjectManager(QObject* parent = nullptr);
    ~ProjectManager() override;

    bool             hasActiveProject() const;
    const Project*   activeProject() const;
    Project*         activeProject();

    QStringList      recentProjects() const;
    void             addRecentProject(const QString& path);
    void             clearRecentProjects();

    /// Programmatic actions (no UI).
    bool createNewProject(const QString& projectName, const QString& filePath, QString* errorOut = nullptr);
    bool openProject(const QString& filePath);
    bool saveActiveProject();
    void closeProject();

    /// Add an existing file as a project member (path is made relative if possible).
    bool addExistingFile(const QString& absolutePath);

    /// UI flows. Each opens a QFileDialog or QInputDialog as appropriate.
    void newProjectInteractive(QWidget* parentWidget);
    void openProjectInteractive(QWidget* parentWidget);
    void addMemberInteractive(QWidget* parentWidget);

    /// Sketch-first creation flow. Ensures a Part Design Body exists in a
    /// new (or active) document, prompts for a base plane, then enters
    /// Sketcher_NewSketch on the chosen plane. Wraps the whole sequence in
    /// a single transaction so a Ctrl+Z fully reverts.
    void newSketchInteractive(QWidget* parentWidget);

    /// Open all member documents (FCStd) in FreeCAD.
    void openAllMemberDocuments();

Q_SIGNALS:
    void projectChanged();
    void recentProjectsChanged();

private:
    void loadRecentList();
    void persistRecentList();

    std::unique_ptr<Project> active_;
    QStringList              recent_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_PROJECTMANAGER_H
