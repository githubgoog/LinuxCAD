// SPDX-License-Identifier: LGPL-2.1-or-later
//
// LinuxCAD shell - public entry point.
//
// Hooks into the existing Gui::MainWindow to install the LinuxCAD-branded
// chrome (top bar, ribbon, theme) without
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
class Ribbon;
class Theme;
class CommandPalette;
class ViewWidgetsOverlay;
class SuggestionEngine;
class GhostToast;
class Provider;
class LinuxCadStart;

GuiExport void refreshDockChromeTint();

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
    Ribbon*             ribbon() const { return ribbon_; }
    Theme*              theme() const { return theme_; }
    CommandPalette*     commandPalette() const { return commandPalette_; }
    SuggestionEngine*   suggestionEngine() const { return suggestionEngine_; }
    GhostToast*         ghostToast() const { return ghostToast_; }
    Provider*           aiProvider() const { return aiProvider_; }

    /// Recreate the AI Provider from settings and attach it to SuggestionEngine.
    void reloadAiProvider();

private:
    friend GuiExport void install(Gui::MainWindow* mw);

    Shell() = default;
    ~Shell() = default;
    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;

    bool anyDocumentOpen() const;
    void ensureStartViewInstalled();
    void refreshStartViewVisibility();

    Gui::MainWindow*    mainWindow_      = nullptr;
    TopBar*             topBar_          = nullptr;
    Ribbon*             ribbon_          = nullptr;
    Theme*              theme_           = nullptr;
    CommandPalette*     commandPalette_  = nullptr;
    LinuxCadStart*      startView_       = nullptr;
    ViewWidgetsOverlay* viewWidgets_     = nullptr;
    SuggestionEngine*   suggestionEngine_= nullptr;
    GhostToast*         ghostToast_      = nullptr;
    Provider*           aiProvider_      = nullptr;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_SHELL_H
