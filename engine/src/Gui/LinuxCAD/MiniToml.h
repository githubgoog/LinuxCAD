// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_MINITOML_H
#define GUI_LINUXCAD_MINITOML_H

#include <FCGlobal.h>
#include <QHash>
#include <QString>
#include <QStringList>

namespace Gui {
namespace LinuxCAD {

/// Minimal TOML-subset reader for LinuxCAD theme manifests.
///
/// Intentionally restricted to the schema we ship: comments starting with
/// '#', \c [section] headers, and \c key = "string-value" pairs with
/// double-quoted string values. Arrays, inline tables, multi-line strings
/// and non-string scalar types are all rejected so a manifest authoring
/// mistake fails loudly instead of silently producing surprising data.
class GuiExport MiniToml
{
public:
    /// Parse \p text. Returns true on success; on failure \p errorOut is
    /// set to a string of the form \c "line N: <reason>".
    bool parse(const QString& text, QString* errorOut = nullptr);

    /// Read a string value from \c [section] \p key. Returns
    /// \p defaultValue if the section or key is missing.
    QString getString(const QString& section,
                      const QString& key,
                      const QString& defaultValue = QString()) const;

    /// All keys defined in \p section, in insertion order is not guaranteed.
    /// Returns an empty list if the section does not exist.
    QStringList keysIn(const QString& section) const;

    /// True if \p section appears in the parsed document at all (even if
    /// it has no keys).
    bool hasSection(const QString& section) const;

private:
    QHash<QString, QHash<QString, QString>> data_;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_MINITOML_H
