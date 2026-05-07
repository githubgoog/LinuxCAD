// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QSettings>
#include <QString>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Command.h>

#include "Shortcuts.h"

namespace Gui {
namespace LinuxCAD {

namespace {

constexpr const char* kFlagKey = "LinuxCAD/ShortcutsAppliedV1";

// Curated, Fusion-aligned shortcut map. Plain single letters and number-row
// punctuation only; we deliberately avoid Ctrl/Alt-modified keys so we don't
// collide with FreeCAD's many existing combos (Ctrl+S, Ctrl+Z, ...).
const std::vector<ShortcutEntry>& curatedRef()
{
    static const std::vector<ShortcutEntry> kEntries = {
        // Modeling
        {"Sketcher_NewSketch",   "S",        "Modeling", "New Sketch"},
        {"PartDesign_Pad",       "E",        "Modeling", "Pad / Extrude"},
        {"PartDesign_Pocket",    "P",        "Modeling", "Pocket"},
        {"PartDesign_Fillet",    "F",        "Modeling", "Fillet"},
        {"PartDesign_Chamfer",   "C",        "Modeling", "Chamfer"},
        {"PartDesign_Draft",     "D",        "Modeling", "Draft"},
        {"PartDesign_Revolution","R",        "Modeling", "Revolve"},

        // View
        {"Std_ViewHome",         "H",        "View",     "Home view"},
        {"Std_ViewFitAll",       "Shift+F",  "View",     "Fit all"},
        {"Std_ViewIsometric",    "0",        "View",     "Isometric"},
        {"Std_ViewFront",        "1",        "View",     "Front"},
        {"Std_ViewTop",          "2",        "View",     "Top"},
        {"Std_ViewRight",        "3",        "View",     "Right"},
        {"Std_OrthographicCamera","/",       "View",     "Toggle Orthographic"},

        // Selection / files
        {"Std_SelectAll",        "Ctrl+A",   "Selection","Select all"},
        {"Std_BoxSelection",     "B",        "Selection","Box select"},

        // Files (kept in cheat sheet only, FreeCAD owns these by default)
        {"Std_New",              "Ctrl+N",   "Files",    "New document"},
        {"Std_Open",             "Ctrl+O",   "Files",    "Open"},
        {"Std_Save",             "Ctrl+S",   "Files",    "Save"},
        {"Std_SaveAs",           "Ctrl+Shift+S","Files", "Save as"},
        {"Std_Undo",             "Ctrl+Z",   "Files",    "Undo"},
        {"Std_Redo",             "Ctrl+Shift+Z","Files", "Redo"},

        // LinuxCAD chrome
        {"Std_DlgPreferences",   "Ctrl+,",   "LinuxCAD", "Preferences"},
    };
    return kEntries;
}

QString g_lastCommandName;

bool isOurCommand(const ShortcutEntry& e)
{
    return e.category != nullptr
        && (std::string(e.category) == "Modeling"
            || std::string(e.category) == "View"
            || std::string(e.category) == "Selection"
            || std::string(e.category) == "LinuxCAD");
}

void writeShortcuts()
{
    auto* app = Gui::Application::Instance;
    if (app == nullptr) {
        return;
    }
    auto& mgr = app->commandManager();
    int applied = 0;
    int missing = 0;
    for (const auto& e : curatedRef()) {
        if (!isOurCommand(e)) {
            continue;  // leave Files category alone; FreeCAD owns those
        }
        auto* cmd = mgr.getCommandByName(e.commandName);
        if (cmd == nullptr) {
            ++missing;
            continue;
        }
        cmd->setShortcut(QString::fromLatin1(e.keySequence));
        ++applied;
    }
    Base::Console().log("LinuxCAD: applied %d curated shortcuts (%d commands missing)\n",
                         applied, missing);
}

} // namespace

void Shortcuts::applyDefaults()
{
    QSettings s;
    if (s.value(QString::fromLatin1(kFlagKey), false).toBool()) {
        return;
    }
    writeShortcuts();
    s.setValue(QString::fromLatin1(kFlagKey), true);
}

void Shortcuts::applyForce()
{
    writeShortcuts();
    QSettings s;
    s.setValue(QString::fromLatin1(kFlagKey), true);
}

const std::vector<ShortcutEntry>& Shortcuts::curated()
{
    return curatedRef();
}

void Shortcuts::repeatLast()
{
    if (g_lastCommandName.isEmpty()) {
        return;
    }
    if (auto* app = Gui::Application::Instance) {
        if (auto* cmd = app->commandManager().getCommandByName(g_lastCommandName.toUtf8().constData())) {
            cmd->invoke(0);
        }
    }
}

void Shortcuts::recordExecuted(const QString& commandName)
{
    if (!commandName.isEmpty()) {
        g_lastCommandName = commandName;
    }
}

} // namespace LinuxCAD
} // namespace Gui
