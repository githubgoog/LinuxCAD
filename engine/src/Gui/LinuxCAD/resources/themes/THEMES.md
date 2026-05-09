# LinuxCAD Themes

LinuxCAD ships with a small theme system: a TOML manifest declares the
palette and a couple of metadata fields, and the application generates the
QSS from a built-in template. You can author and drop in your own theme
without rebuilding LinuxCAD.

## Where themes live

User themes are loaded from a per-user directory; bundled themes live in
the application resources.

| Platform | User theme directory                              |
| -------- | -------------------------------------------------- |
| Linux    | `~/.config/LinuxCAD/themes/<id>.theme.toml`        |
| macOS    | `~/Library/Application Support/LinuxCAD/themes/`   |
| Windows  | `%APPDATA%\LinuxCAD\themes\<id>.theme.toml`        |

A user theme whose `id` matches a bundled theme overrides the bundled one.

## Bundled themes

| ID                | Name            | Notes                              |
| ----------------- | --------------- | ---------------------------------- |
| `charcoal-amber`  | Charcoal Amber  | Default. Dark charcoal + amber.    |
| `light-amber`     | Light Amber     | Light counterpart with amber.      |
| `system`          | System          | No QSS; falls back to platform.    |

## File format

Each theme is a single `.theme.toml` file. The reader supports a strict
TOML subset:

- `# comments`
- `[section]` headers
- `key = "double-quoted-string"` values

Arrays, inline tables, and unquoted scalars are not supported.

### Required `[theme]` keys

| Key           | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| `name`        | Display name shown in the picker.                            |
| `id`          | Stable identifier, also the file stem.                       |
| `author`      | Author or organization.                                      |
| `description` | One-line description shown as a tooltip.                     |
| `font_family` | CSS-style font stack. Empty string means "use platform default". |

### Required `[colors]` keys

| Key                | Used for                                                      |
| ------------------ | ------------------------------------------------------------- |
| `bg_primary`       | Canvas / lowest elevation tier.                               |
| `bg_secondary`     | Standard surface tier (panels, dialogs).                      |
| `bg_hover`         | Raised tier and hover backgrounds.                            |
| `border_subtle`    | 1px hairline borders between adjacent surfaces.               |
| `border_strong`    | Stronger separators and inactive control outlines.            |
| `text_primary`     | Primary foreground text.                                      |
| `text_muted`       | Secondary, less important text (subtitles, hints).            |
| `text_dim`         | Disabled or placeholder text.                                 |
| `accent`           | Primary brand accent (LinuxCAD: amber `#F59E0B`).             |
| `accent_hover`     | Accent under hover.                                           |
| `accent_pressed`   | Accent under press / active.                                  |
| `scrollbar_bg`     | Scrollbar gutter color.                                       |
| `scrollbar_thumb`  | Scrollbar thumb color.                                        |
| `selection_bg`     | Selection background in lists/text.                           |
| `selection_text`   | Selection foreground.                                         |
| `ai_idle`          | AI copilot status: idle.                                      |
| `ai_thinking`      | AI copilot status: thinking / streaming.                      |
| `ai_error`         | AI copilot status: error.                                     |
| `ai_disabled`      | AI copilot status: disabled / offline.                        |

All color values are CSS-style `#RRGGBB` strings.

### Optional `extra_qss`

`extra_qss` is a single quoted string (no multi-line strings supported)
appended verbatim to the generated QSS. Use it for narrow tweaks; prefer
the color tokens above whenever possible.

## Authoring a new theme

1. Copy `charcoal-amber.theme.toml` next to itself and rename the copy to
   `<your-id>.theme.toml`.
2. Edit the `[theme]` block: pick a unique `id`, pick a `name`, set the
   `author` and `description`.
3. Edit the `[colors]` block. The reader complains line-by-line if a value
   is not a quoted string.
4. Drop the file in the user theme directory for your platform (see
   above). No restart is required when authored interactively, but a
   restart is the simplest way to verify the change loads cleanly.
5. Open the user-menu **Themes...** dialog, pick your theme, and click
   **Apply**.

The picker dialog shows live previews where supported; hover the entry to
see the description and click any swatch to inspect the color value.

## Special case: `system`

The `system` theme is a sentinel. Its `[colors]` table is empty, which the
loader treats as "do not apply LinuxCAD QSS at all" — Qt falls back to
whatever style the host platform provides. Useful as a debugging escape
hatch.
