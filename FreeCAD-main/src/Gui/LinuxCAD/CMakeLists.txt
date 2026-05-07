# LinuxCAD shell module.
#
# This subdirectory is included from src/Gui/CMakeLists.txt and contributes
# its sources to the FreeCADGui shared library. It owns the LinuxCAD-branded
# chrome (top bar, project manager dock, welcome screen, theme, command
# palette) without modifying FreeCAD's modeling UI.

set(LINUXCAD_SOURCES
    LinuxCadShell.cpp
    LinuxCadShell.h
    TopBar.cpp
    TopBar.h
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
