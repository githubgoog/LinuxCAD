// SPDX-License-Identifier: LGPL-2.1-or-later
//
// LinuxCAD shell - public entry point.
//
// Hooks into the existing Gui::MainWindow to install the LinuxCAD-branded
// chrome (top bar, project manager dock, welcome screen, theme) without
// touching any of FreeCAD's modeling UI.

#ifndef GUI_LINUXCAD_SHELL_H
#define GUI_LINUXCAD_SHELL_H

#include <FCGlobal.h>

namespace Gui {
class MainWindow;
}

namespace Gui {
namespace LinuxCAD {

class TopBar;
class ProjectManagerDock;
class WelcomeScreen;
class ProjectManager;
class Theme;
class CommandPalette;

/// Install LinuxCAD shell on the given MainWindow.
///
/// Idempotent: calling more than once is a no-op.
/// Safe to call from MainWindow::MainWindow at the end of construction.
GuiExport void install(Gui::MainWindow* mw);

/// Returns the singleton shell, or nullptr if not yet installed.
class Shell;
GuiExport Shell* shell();

class GuiExport Shell
{
public:
    static Shell* instance();

    Gui::MainWindow*    mainWindow() const { return mainWindow_; }
    TopBar*             topBar() const { return topBar_; }
    ProjectManagerDock* projectDock() const { return projectDock_; }
    WelcomeScreen*      welcomeScreen() const { return welcomeScreen_; }
    ProjectManager*     projectManager() const { return projectManager_; }
    Theme*              theme() const { return theme_; }
    CommandPalette*     commandPalette() const { return commandPalette_; }

private:
    friend GuiExport void install(Gui::MainWindow* mw);

    Shell() = default;
    ~Shell() = default;
    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;

    Gui::MainWindow*    mainWindow_    = nullptr;
    TopBar*             topBar_        = nullptr;
    ProjectManagerDock* projectDock_   = nullptr;
    WelcomeScreen*      welcomeScreen_ = nullptr;
    ProjectManager*     projectManager_= nullptr;
    Theme*              theme_         = nullptr;
    CommandPalette*     commandPalette_= nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_SHELL_H
