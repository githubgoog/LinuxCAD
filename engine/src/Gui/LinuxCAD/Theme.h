// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef GUI_LINUXCAD_THEME_H
#define GUI_LINUXCAD_THEME_H

#include <FCGlobal.h>
#include <QHash>
#include <QList>
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
    struct ThemeRecord
    {
        enum class SourceType {
            Bundled,
            User,
        };

        QString id;
        QString name;
        QString author;
        QString description;
        QString fontFamily;
        QHash<QString, QString> colors;
        QString extraQss;
        QString sourcePath;
        SourceType sourceType = SourceType::Bundled;
    };

    enum class Variant {
        Dark,
        Light,
        System,
    };

    explicit Theme(QObject* parent = nullptr);
    ~Theme() override;

    /// Apply the persisted theme (defaults to charcoal-amber).
    void applyDefault();
    void applyTheme(const QString& id);
    void applyVariant(Variant v);

    QString currentThemeId() const { return currentThemeId_; }
    Variant currentVariant() const { return current_; }
    static QString variantToString(Variant v);
    static Variant variantFromString(const QString& s);
    static QString normalizeThemeId(const QString& id);
    static QString defaultThemeId();

    /// Return the QSS string for the user's persisted variant, or empty for
    /// "system". Safe to call before any Theme instance has been constructed
    /// because it reads the persisted setting directly. Used by FreeCAD's
    /// stylesheet pipeline to append our rules so they survive theme reloads.
    static QString currentStylesheet();

    /// Return all discovered themes in effective order.
    static QList<ThemeRecord> discoveredThemes();
    static ThemeRecord themeForId(const QString& id);

    /// Return the QSS string for the requested variant (compatibility API).
    static QString stylesheetFor(Variant v);
    static QString stylesheetForId(const QString& id);

private:
    QString loadStylesheet(Variant v) const;
    static ThemeRecord fallbackRecord();

    Variant current_ = Variant::Dark;
    QString currentThemeId_ = QStringLiteral("charcoal-amber");
};

} // namespace LinuxCAD
} // namespace Gui

#endif // GUI_LINUXCAD_THEME_H
