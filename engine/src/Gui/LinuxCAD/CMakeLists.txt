# LinuxCAD shell module.
#
# This subdirectory is included from src/Gui/CMakeLists.txt and contributes
# its sources to the FreeCADGui shared library. It owns the LinuxCAD-branded
# chrome (top bar, ribbon, project manager dock, welcome screen, theme,
# command palette, navigation polish, AI copilot) without modifying
# FreeCAD's modeling UI.

set(LINUXCAD_SOURCES
    LinuxCadShell.cpp
    LinuxCadShell.h
    TopBar.cpp
    TopBar.h
    Ribbon.cpp
    Ribbon.h
    ProjectManagerDock.cpp
    ProjectManagerDock.h
    WelcomeScreen.cpp
    WelcomeScreen.h
    Project.cpp
    Project.h
    ProjectManager.cpp
    ProjectManager.h
    Theme.cpp
    Theme.h
    CommandPalette.cpp
    CommandPalette.h
    NaviCubeDefaults.cpp
    NaviCubeDefaults.h
    ViewWidgetsOverlay.cpp
    ViewWidgetsOverlay.h
    Shortcuts.cpp
    Shortcuts.h
    CheatSheet.cpp
    CheatSheet.h
    AI/Provider.cpp
    AI/Provider.h
    AI/OpenAIProvider.cpp
    AI/OpenAIProvider.h
    AI/AnthropicProvider.cpp
    AI/AnthropicProvider.h
    AI/ContextBuilder.cpp
    AI/ContextBuilder.h
    AI/ToolRegistry.cpp
    AI/ToolRegistry.h
    AI/SuggestionEngine.cpp
    AI/SuggestionEngine.h
    AI/GhostToast.cpp
    AI/GhostToast.h
    AI/Consent.cpp
    AI/Consent.h
)

set(LINUXCAD_RESOURCES
    resources/linuxcad.qrc
)

# Expose sources and resources to the parent (src/Gui) scope so they can be
# added to the FreeCADGui target there. This keeps our additions co-located
# without forking the parent build target.
set(LINUXCAD_SOURCES         ${LINUXCAD_SOURCES}         PARENT_SCOPE)
set(LINUXCAD_RESOURCES       ${LINUXCAD_RESOURCES}       PARENT_SCOPE)
set(LINUXCAD_INCLUDE_DIR     ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
