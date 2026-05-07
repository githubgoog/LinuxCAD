// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#endif

#include <App/Application.h>
#include <Base/Console.h>

#include "Project.h"
#include "ProjectManager.h"
#include "ProjectManagerDock.h"

namespace Gui {
namespace LinuxCAD {

namespace {

QString iconNameForKind(ProjectMember::Kind k)
{
    switch (k) {
        case ProjectMember::Kind::Part:      return QStringLiteral("Part_Box");
        case ProjectMember::Kind::Assembly:  return QStringLiteral("Assembly_Assembly");
        case ProjectMember::Kind::Drawing:   return QStringLiteral("TechDraw_TreePage");
        case ProjectMember::Kind::Reference: return QStringLiteral("freecad-document");
        case ProjectMember::Kind::Asset:     return QStringLiteral("preferences-general");
        case ProjectMember::Kind::Unknown:   break;
    }
    return QStringLiteral("freecad-document");
}

QString humanKind(ProjectMember::Kind k)
{
    switch (k) {
        case ProjectMember::Kind::Part:      return QObject::tr("Part");
        case ProjectMember::Kind::Assembly:  return QObject::tr("Assembly");
        case ProjectMember::Kind::Drawing:   return QObject::tr("Drawing");
        case ProjectMember::Kind::Reference: return QObject::tr("Reference");
        case ProjectMember::Kind::Asset:     return QObject::tr("Asset");
        case ProjectMember::Kind::Unknown:   return QObject::tr("File");
    }
    return QObject::tr("File");
}

QString groupForKind(ProjectMember::Kind k)
{
    switch (k) {
        case ProjectMember::Kind::Part:      return QObject::tr("Parts");
        case ProjectMember::Kind::Assembly:  return QObject::tr("Assemblies");
        case ProjectMember::Kind::Drawing:   return QObject::tr("Drawings");
        case ProjectMember::Kind::Reference: return QObject::tr("References");
        case ProjectMember::Kind::Asset:     return QObject::tr("Assets");
        case ProjectMember::Kind::Unknown:   return QObject::tr("Other");
    }
    return QObject::tr("Other");
}

} // namespace

ProjectManagerDock::ProjectManagerDock(ProjectManager* manager, QWidget* parent)
    : QDockWidget(tr("Project"), parent)
    , manager_(manager)
{
    setObjectName(QStringLiteral("LinuxCadProjectDock"));
    setProperty("linuxcadRole", QStringLiteral("project-dock"));
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    buildUi();

    if (manager_) {
        connect(manager_, &ProjectManager::projectChanged, this, &ProjectManagerDock::rebuild);
    }

    rebuild();
}

ProjectManagerDock::~ProjectManagerDock() = default;

void ProjectManagerDock::buildUi()
{
    auto* container = new QWidget(this);
    container->setProperty("linuxcadRole", QStringLiteral("project-dock-body"));
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    header_ = new QLabel(tr("No project open"), container);
    header_->setProperty("linuxcadRole", QStringLiteral("project-header"));
    QFont f = header_->font();
    f.setPointSizeF(f.pointSizeF() * 1.15);
    f.setBold(true);
    header_->setFont(f);
    layout->addWidget(header_);

    subHeader_ = new QLabel(QString(), container);
    subHeader_->setProperty("linuxcadRole", QStringLiteral("project-sub-header"));
    subHeader_->setWordWrap(true);
    layout->addWidget(subHeader_);

    filter_ = new QLineEdit(container);
    filter_->setPlaceholderText(tr("Filter members..."));
    filter_->setClearButtonEnabled(true);
    layout->addWidget(filter_);
    connect(filter_, &QLineEdit::textChanged, this, &ProjectManagerDock::onFilterChanged);

    tree_ = new QTreeWidget(container);
    tree_->setColumnCount(2);
    tree_->setHeaderLabels({tr("Name"), tr("Type")});
    tree_->header()->setStretchLastSection(false);
    tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree_->setRootIsDecorated(true);
    tree_->setUniformRowHeights(true);
    layout->addWidget(tree_, 1);
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, &ProjectManagerDock::onItemDoubleClicked);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    addBtn_   = new QPushButton(tr("Add..."), container);
    saveBtn_  = new QPushButton(tr("Save"), container);
    closeBtn_ = new QPushButton(tr("Close"), container);

    connect(addBtn_,   &QPushButton::clicked, this, &ProjectManagerDock::onAddFile);
    connect(saveBtn_,  &QPushButton::clicked, this, &ProjectManagerDock::onSaveProject);
    connect(closeBtn_, &QPushButton::clicked, this, &ProjectManagerDock::onCloseProject);

    btnRow->addWidget(addBtn_);
    btnRow->addWidget(saveBtn_);
    btnRow->addWidget(closeBtn_);
    layout->addLayout(btnRow);

    setWidget(container);
}

void ProjectManagerDock::rebuild()
{
    if (!manager_ || !manager_->hasActiveProject()) {
        header_->setText(tr("No project open"));
        subHeader_->setText(tr("Use Project ▸ New / Open from the top bar to start."));
        addBtn_->setEnabled(false);
        saveBtn_->setEnabled(false);
        closeBtn_->setEnabled(false);
        if (tree_) {
            tree_->clear();
        }
        return;
    }

    addBtn_->setEnabled(true);
    saveBtn_->setEnabled(true);
    closeBtn_->setEnabled(true);

    const Project* p = manager_->activeProject();
    header_->setText(p->name().isEmpty() ? tr("Untitled Project") : p->name());
    subHeader_->setText(p->filePath());

    populateMembers();
    applyFilter();
}

void ProjectManagerDock::populateMembers()
{
    tree_->clear();
    if (!manager_ || !manager_->hasActiveProject()) {
        return;
    }
    const Project* p = manager_->activeProject();

    // Group items by member kind.
    QMap<ProjectMember::Kind, QTreeWidgetItem*> groups;

    auto getGroup = [&](ProjectMember::Kind kind) {
        auto it = groups.find(kind);
        if (it != groups.end()) {
            return it.value();
        }
        auto* g = new QTreeWidgetItem(tree_);
        g->setText(0, groupForKind(kind));
        g->setFirstColumnSpanned(true);
        QFont f = g->font(0);
        f.setBold(true);
        g->setFont(0, f);
        g->setExpanded(true);
        g->setFlags(g->flags() & ~Qt::ItemIsSelectable);
        groups.insert(kind, g);
        return g;
    };

    for (const auto& m : p->members()) {
        auto* parent = getGroup(m.kind);
        auto* child = new QTreeWidgetItem(parent);
        child->setText(0, m.name);
        child->setText(1, humanKind(m.kind));
        child->setToolTip(0, p->absolutePathFor(m));
        child->setData(0, Qt::UserRole, p->absolutePathFor(m));
    }

    tree_->expandAll();
}

void ProjectManagerDock::applyFilter()
{
    const QString needle = filter_ ? filter_->text().trimmed() : QString();
    for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
        auto* group = tree_->topLevelItem(i);
        int visibleChildren = 0;
        for (int j = 0; j < group->childCount(); ++j) {
            auto* child = group->child(j);
            const bool match = needle.isEmpty()
                || child->text(0).contains(needle, Qt::CaseInsensitive)
                || child->text(1).contains(needle, Qt::CaseInsensitive);
            child->setHidden(!match);
            if (match) {
                ++visibleChildren;
            }
        }
        group->setHidden(visibleChildren == 0);
    }
}

void ProjectManagerDock::onFilterChanged(const QString& /*text*/)
{
    applyFilter();
}

void ProjectManagerDock::onItemDoubleClicked(QTreeWidgetItem* item, int /*col*/)
{
    if (!item) {
        return;
    }
    const QString abs = item->data(0, Qt::UserRole).toString();
    if (abs.isEmpty() || !QFileInfo::exists(abs)) {
        return;
    }
    try {
        App::GetApplication().openDocument(abs.toUtf8().constData());
    }
    catch (...) {
        Base::Console().error("LinuxCAD: failed to open '%s'\n", abs.toUtf8().constData());
    }
}

void ProjectManagerDock::onAddFile()
{
    if (manager_) {
        manager_->addMemberInteractive(this);
    }
}

void ProjectManagerDock::onSaveProject()
{
    if (manager_) {
        manager_->saveActiveProject();
    }
}

void ProjectManagerDock::onCloseProject()
{
    if (manager_) {
        manager_->closeProject();
    }
}

} // namespace LinuxCAD
} // namespace Gui
