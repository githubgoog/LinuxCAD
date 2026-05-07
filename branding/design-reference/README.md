# LinuxCAD Design Reference

This folder captures the visual design language extracted from the original
Electron/React `frontend/` so that the new Qt-based shell (forked from FreeCAD)
can carry it forward.

## Design language summary

- Modern, minimal, dense-but-uncluttered, pro-tooling vibe.
- Two themes: light and dark (dark is the default for shipping).
- Typography: system-default UI font on each OS, monospace for technical readouts.
- Iconography: line icons (Lucide-inspired), 1.5px stroke, neutral hue,
  filled accent only on active state.
- Accent color: blue family (matches FreeCAD's existing splash color #418FDE).
- Surfaces are flat with subtle 1px borders; corners rounded (4-6px).
- Hover/active feedback uses background tint, not heavy shadows.

## Color tokens (used by `branding/themes/*.qss`)

Dark theme:

- `--bg-0`            #0F1216  (window background)
- `--bg-1`            #161A20  (top bar, dock backgrounds)
- `--bg-2`            #1E232B  (panels, hovered surfaces)
- `--bg-3`            #2A313B  (active surfaces, selected rows)
- `--border`          #2C333D
- `--text-primary`    #E6EAF0
- `--text-secondary`  #9AA3AF
- `--text-muted`      #6B7280
- `--accent`          #418FDE  (FreeCAD splash blue)
- `--accent-strong`   #5BA8F2
- `--danger`          #CA333B
- `--success`         #4CAF50

Light theme:

- `--bg-0`            #FAFBFC
- `--bg-1`            #FFFFFF
- `--bg-2`            #F1F3F5
- `--bg-3`            #E4E8EE
- `--border`          #DCE0E5
- `--text-primary`    #1A1F26
- `--text-secondary`  #4B5563
- `--text-muted`      #8A93A1
- `--accent`          #2F7BD0
- `--accent-strong`   #1D5FAA
- `--danger`          #C43338
- `--success`         #2D8A47

## Layout intent

- Top bar (single row, 44-48px tall) replaces FreeCAD's QMenuBar + standard
  toolbar. Contains: app icon, project menu, workbench switcher, command
  palette / quick search, undo/redo, save indicator, user area.
- Left dock: LinuxCAD Project Manager (recent projects, project tree).
- Right dock: FreeCAD's existing tree + property editor, restyled.
- Center: FreeCAD's 3D viewport, untouched.
- Bottom: status bar, restyled.

## What we do not change

- The 3D viewport, sketcher, PartDesign and other workbench task panels,
  preferences pages, or the .FCStd file format.

## Source intent

The original concept lived in `frontend/src/components/Viewport3D/` (broken into
overlay/control/scene subcomponents per `VIEWPORT_OPTIMIZATION.md`) and a
`DemoPanel`. None of that code is carried forward: only the design intent is.
