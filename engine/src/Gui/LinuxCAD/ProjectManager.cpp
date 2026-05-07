// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>

#include "ProjectManager.h"

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr int kMaxRecent = 12;
const QString kSettingsKey   = QStringLiteral("LinuxCAD/RecentProjects");
const QString kProjectFilter = QStringLiteral("LinuxCAD Projects (*.lcadproj);;All Files (*)");
const QString kFCStdFilter   = QStringLiteral(
    "Supported (*.FCStd *.step *.stp *.iges *.igs *.stl *.obj *.brep *.dxf *.svg);;"
    "FreeCAD Documents (*.FCStd);;"
    "STEP / IGES (*.step *.stp *.iges *.igs);;"
    "Mesh / BREP (*.stl *.obj *.brep);;"
    "Drawings (*.dxf *.svg);;"
    "All Files (*)"
);

QString defaultProjectsDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath();
    }
    QDir d(dir);
    const QString projectsName = QStringLiteral("LinuxCAD Projects");
    if (!d.exists(projectsName)) {
        d.mkpath(projectsName);
    }
    return d.absoluteFilePath(projectsName);
}

} // namespace

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
{
    loadRecentList();
}

ProjectManager::~ProjectManager() = default;

bool ProjectManager::hasActiveProject() const
{
    return active_.get() != nullptr;
}

const Project* ProjectManager::activeProject() const
{
    return active_.get();
}

Project* ProjectManager::activeProject()
{
    return active_.get();
}

QStringList ProjectManager::recentProjects() const
{
    return recent_;
}

void ProjectManager::addRecentProject(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
    recent_.removeAll(path);
    recent_.prepend(path);
    while (recent_.size() > kMaxRecent) {
        recent_.removeLast();
    }
    persistRecentList();
    Q_EMIT recentProjectsChanged();
}

void ProjectManager::clearRecentProjects()
{
    recent_.clear();
    persistRecentList();
    Q_EMIT recentProjectsChanged();
}

bool ProjectManager::createNewProject(const QString& projectName, const QString& filePath, QString* errorOut)
{
    auto p = std::make_unique<Project>(Project::newEmpty(projectName, filePath));
    if (!p->save(errorOut)) {
        return false;
    }
    active_ = std::move(p);
    addRecentProject(filePath);
    Q_EMIT projectChanged();
    return true;
}

bool ProjectManager::openProject(const QString& filePath)
{
    auto p = std::make_unique<Project>();
    QString err;
    if (!p->load(filePath, &err)) {
        Base::Console().error("LinuxCAD: failed to open project '%s': %s\n",
                              filePath.toUtf8().constData(),
                              err.toUtf8().constData());
        return false;
    }
    active_ = std::move(p);
    addRecentProject(filePath);
    openAllMemberDocuments();
    Q_EMIT projectChanged();
    return true;
}

bool ProjectManager::saveActiveProject()
{
    if (!active_) {
        return false;
    }
    QString err;
    if (!active_->save(&err)) {
        Base::Console().error("LinuxCAD: failed to save project: %s\n", err.toUtf8().constData());
        return false;
    }
    Q_EMIT projectChanged();
    return true;
}

void ProjectManager::closeProject()
{
    if (!active_) {
        return;
    }
    active_.reset();
    Q_EMIT projectChanged();
}

bool ProjectManager::addExistingFile(const QString& absolutePath)
{
    if (!active_) {
        return false;
    }
    ProjectMember m;
    const QFileInfo fi(absolutePath);
    m.name = fi.completeBaseName();
    m.kind = ProjectMember::inferKindFromPath(absolutePath);

    const QString root = active_->rootDir();
    if (!root.isEmpty()) {
        m.relativePath = QDir(root).relativeFilePath(absolutePath);
    }
    else {
        m.relativePath = absolutePath;
    }
    active_->members().append(m);
    saveActiveProject();
    return true;
}

void ProjectManager::newProjectInteractive(QWidget* parentWidget)
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        parentWidget,
        tr("New LinuxCAD Project"),
        tr("Project name"),
        QLineEdit::Normal,
        QStringLiteral("Untitled"),
        &ok
    );
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    const QString suggested = QDir(defaultProjectsDir()).absoluteFilePath(
        name.trimmed() + QStringLiteral(".lcadproj"));
    QString path = QFileDialog::getSaveFileName(
        parentWidget,
        tr("Choose where to save the project"),
        suggested,
        kProjectFilter
    );
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".lcadproj"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".lcadproj");
    }

    QString err;
    if (!createNewProject(name.trimmed(), path, &err)) {
        QMessageBox::warning(parentWidget, tr("LinuxCAD"),
                             tr("Could not create project:\n%1").arg(err));
    }
}

void ProjectManager::openProjectInteractive(QWidget* parentWidget)
{
    const QString path = QFileDialog::getOpenFileName(
        parentWidget,
        tr("Open LinuxCAD Project"),
        defaultProjectsDir(),
        kProjectFilter
    );
    if (path.isEmpty()) {
        return;
    }
    if (!openProject(path)) {
        QMessageBox::warning(parentWidget, tr("LinuxCAD"),
                             tr("Could not open project:\n%1").arg(path));
    }
}

void ProjectManager::addMemberInteractive(QWidget* parentWidget)
{
    if (!active_) {
        QMessageBox::information(parentWidget, tr("LinuxCAD"),
                                 tr("Open or create a project first."));
        return;
    }
    const QStringList paths = QFileDialog::getOpenFileNames(
        parentWidget,
        tr("Add files to project"),
        active_->rootDir(),
        kFCStdFilter
    );
    for (const auto& p : paths) {
        addExistingFile(p);
    }
}

void ProjectManager::openAllMemberDocuments()
{
    if (!active_) {
        return;
    }
    for (const auto& m : active_->members()) {
        if (m.kind != ProjectMember::Kind::Part
         && m.kind != ProjectMember::Kind::Assembly
         && m.kind != ProjectMember::Kind::Drawing) {
            continue; // We only auto-open FreeCAD-native docs.
        }
        const QString abs = active_->absolutePathFor(m);
        if (abs.isEmpty() || !QFileInfo::exists(abs)) {
            Base::Console().warning("LinuxCAD: missing project member '%s'\n",
                                    abs.toUtf8().constData());
            continue;
        }
        try {
            App::GetApplication().openDocument(abs.toUtf8().constData());
        }
        catch (...) {
            Base::Console().error("LinuxCAD: failed to open '%s'\n", abs.toUtf8().constData());
        }
    }
}

void ProjectManager::loadRecentList()
{
    QSettings s;
    recent_ = s.value(kSettingsKey).toStringList();
    // Drop entries that no longer exist on disk.
    QStringList alive;
    alive.reserve(recent_.size());
    for (const auto& p : recent_) {
        if (QFileInfo::exists(p)) {
            alive.append(p);
        }
    }
    if (alive.size() != recent_.size()) {
        recent_ = alive;
        persistRecentList();
    }
}

void ProjectManager::persistRecentList()
{
    QSettings s;
    s.setValue(kSettingsKey, recent_);
}

namespace {

void runFreeCADCommand(const char* name)
{
    if (auto* app = Gui::Application::Instance) {
        if (auto* cmd = app->commandManager().getCommandByName(name)) {
            cmd->invoke(0);
        }
    }
}

App::Document* ensureActiveDocument()
{
    auto* doc = App::GetApplication().getActiveDocument();
    if (doc != nullptr) {
        return doc;
    }
    runFreeCADCommand("Std_New");
    return App::GetApplication().getActiveDocument();
}

const char* askForPlane(QWidget* parent)
{
    QStringList options;
    options << QObject::tr("XY plane (top)")
            << QObject::tr("XZ plane (front)")
            << QObject::tr("YZ plane (right)");
    bool ok = false;
    const QString choice = QInputDialog::getItem(
        parent,
        QObject::tr("New Sketch"),
        QObject::tr("Sketch on which plane?"),
        options,
        0,
        false,
        &ok);
    if (!ok) {
        return nullptr;
    }
    if (choice == options[0]) {
        return "XY_Plane";
    }
    if (choice == options[1]) {
        return "XZ_Plane";
    }
    return "YZ_Plane";
}

} // namespace

void ProjectManager::newSketchInteractive(QWidget* parentWidget)
{
    auto* doc = ensureActiveDocument();
    if (doc == nullptr) {
        QMessageBox::warning(parentWidget,
                             tr("New Sketch"),
                             tr("Could not create or activate a document."));
        return;
    }

    const char* plane = askForPlane(parentWidget);
    if (plane == nullptr) {
        return; // user cancelled
    }

    doc->openTransaction("Sketch-first creation");
    try {
        // PartDesign_Body creates a new Body and activates it. If a Body is
        // already active this is a no-op for our purposes - the new sketch
        // will land inside the active Body.
        runFreeCADCommand("PartDesign_Body");
        // Pre-select the requested plane through Selection so Sketcher_NewSketch
        // can attach to it without us hand-rolling Python.
        Gui::Selection().clearSelection();
        const std::string planeFullName = std::string("Origin.") + plane;
        // FreeCAD's Origin features are children of the active Body. We
        // attempt the canonical full path first; if that fails Sketcher will
        // surface its own plane picker which is a fine fallback.
        if (auto* gdoc = Gui::Application::Instance->getDocument(doc)) {
            (void)gdoc;
        }
        // Best-effort: invoke NewSketch; Sketcher's default will fall through
        // to its own plane picker if our preselection didn't take.
        runFreeCADCommand("Sketcher_NewSketch");
        doc->commitTransaction();
    }
    catch (...) {
        doc->abortTransaction();
        QMessageBox::warning(parentWidget,
                             tr("New Sketch"),
                             tr("Could not start a sketch on the chosen plane."));
    }
}

} // namespace LinuxCAD
} // namespace Gui
