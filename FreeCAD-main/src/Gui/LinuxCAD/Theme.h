// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_THEME_H
#define GUI_LINUXCAD_THEME_H

#include <FCGlobal.h>
#include <QObject>
#include <QString>

namespace Gui {
namespace LinuxCAD {

/// Loads and applies LinuxCAD QSS themes ("dark" or "light").
///
/// The themes themselves live as Qt resources under :/linuxcad/themes/*.qss
/// and target widgets via the property `linuxcadRole`.
class GuiExport Theme : public QObject
{
    Q_OBJECT

public:
    enum class Variant {
        Dark,
        Light,
        System,
    };

    explicit Theme(QObject* parent = nullptr);
    ~Theme() override;

    /// Apply the persisted theme (defaults to dark).
    void applyDefault();
    void applyVariant(Variant v);

    Variant currentVariant() const { return current_; }
    static QString variantToString(Variant v);
    static Variant variantFromString(const QString& s);

private:
    QString loadStylesheet(Variant v) const;

    Variant current_ = Variant::Dark;
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_THEME_H
