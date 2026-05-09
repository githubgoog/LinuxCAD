// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QMap>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QStandardPaths>
#endif

#include <Gui/Application.h>

#include "LinuxCadShell.h"
#include "MiniToml.h"
#include "Theme.h"

namespace Gui {
namespace LinuxCAD {

namespace {
const QString kSettingsKey = QStringLiteral("LinuxCAD/Theme");
const QString kDefaultThemeId = QStringLiteral("charcoal-amber");
const QString kSystemThemeId = QStringLiteral("system");
const QString kBaseTemplatePath = QStringLiteral(":/linuxcad/themes/base.qss.tmpl");
const QString kLegacyDark = QStringLiteral("dark");
const QString kLegacyLight = QStringLiteral("light");
const QString kLightAmber = QStringLiteral("light-amber");

QString readTextFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

QString normalizeId(const QString& raw)
{
    const QString id = raw.trimmed().toLower();
    if (id.isEmpty()) {
        return kDefaultThemeId;
    }
    if (id == kLegacyDark) {
        return kDefaultThemeId;
    }
    if (id == kLegacyLight) {
        return kLightAmber;
    }
    return id;
}

QString firstFontFamily(const QString& family)
{
    const QStringList pieces = family.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (pieces.isEmpty()) {
        return QString();
    }
    QString first = pieces.first().trimmed();
    if (first.startsWith(QLatin1Char('"')) && first.endsWith(QLatin1Char('"')) && first.size() > 1) {
        first = first.mid(1, first.size() - 2);
    }
    return first;
}

Theme::ThemeRecord makeRecord(const QString& path, Theme::ThemeRecord::SourceType sourceType)
{
    Theme::ThemeRecord rec;
    rec.sourcePath = path;
    rec.sourceType = sourceType;

    const QString text = readTextFile(path);
    if (text.isEmpty()) {
        return rec;
    }

    MiniToml parser;
    QString error;
    if (!parser.parse(text, &error) || !parser.hasSection(QStringLiteral("theme"))) {
        return rec;
    }

    rec.id = normalizeId(parser.getString(QStringLiteral("theme"), QStringLiteral("id")));
    rec.name = parser.getString(QStringLiteral("theme"), QStringLiteral("name"), rec.id);
    rec.author = parser.getString(QStringLiteral("theme"), QStringLiteral("author"));
    rec.description = parser.getString(QStringLiteral("theme"), QStringLiteral("description"));
    rec.fontFamily = parser.getString(QStringLiteral("theme"), QStringLiteral("font_family"));
    rec.extraQss = parser.getString(QStringLiteral("colors"), QStringLiteral("extra_qss"));

    const QStringList keys = parser.keysIn(QStringLiteral("colors"));
    for (const QString& key : keys) {
        if (key == QStringLiteral("extra_qss")) {
            continue;
        }
        rec.colors.insert(key, parser.getString(QStringLiteral("colors"), key));
    }
    return rec;
}

QStringList bundledManifestPaths()
{
    QStringList out;
    const QDir dir(QStringLiteral(":/linuxcad/themes"));
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("*.theme.toml"),
                                            QDir::Files,
                                            QDir::Name);
    for (const QString& file : files) {
        out.append(QStringLiteral(":/linuxcad/themes/") + file);
    }
    return out;
}

QString userThemeDirectory()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (root.isEmpty()) {
        root = QDir::homePath() + QStringLiteral("/.config/LinuxCAD");
    }
    return QDir(root).filePath(QStringLiteral("themes"));
}

QStringList userManifestPaths()
{
    const QDir dir(userThemeDirectory());
    if (!dir.exists()) {
        return {};
    }
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("*.theme.toml"),
                                            QDir::Files,
                                            QDir::Name);
    QStringList out;
    for (const QString& file : files) {
        out.append(dir.absoluteFilePath(file));
    }
    return out;
}

QList<Theme::ThemeRecord> discoverThemes()
{
    QMap<QString, Theme::ThemeRecord> merged;
    for (const QString& path : bundledManifestPaths()) {
        const Theme::ThemeRecord rec = makeRecord(path, Theme::ThemeRecord::SourceType::Bundled);
        if (!rec.id.isEmpty()) {
            merged.insert(rec.id, rec);
        }
    }
    for (const QString& path : userManifestPaths()) {
        const Theme::ThemeRecord rec = makeRecord(path, Theme::ThemeRecord::SourceType::User);
        if (!rec.id.isEmpty()) {
            // User themes override bundled themes with the same id.
            merged.insert(rec.id, rec);
        }
    }
    return merged.values();
}

QString substituteTokens(const QString& templ,
                        const QHash<QString, QString>& colors,
                        const QHash<QString, QString>& fallbackColors)
{
    QString out = templ;
    static const QRegularExpression tokenRe(QStringLiteral("\\{\\{([a-zA-Z0-9_]+)\\}\\}"));
    QRegularExpressionMatchIterator it = tokenRe.globalMatch(templ);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString token = m.captured(1);
        QString value = colors.value(token);
        if (value.isEmpty()) {
            value = fallbackColors.value(token);
        }
        if (value.isEmpty()) {
            value = QStringLiteral("#000000");
        }
        out.replace(m.captured(0), value);
    }
    return out;
}

QString legacyQssPathForId(const QString& id)
{
    if (id == kLightAmber) {
        return QStringLiteral(":/linuxcad/themes/light.qss");
    }
    return QStringLiteral(":/linuxcad/themes/dark.qss");
}
} // namespace

Theme::Theme(QObject* parent)
    : QObject(parent)
{
}

Theme::~Theme() = default;

QString Theme::variantToString(Variant v)
{
    switch (v) {
        case Variant::Dark:   return QStringLiteral("dark");
        case Variant::Light:  return QStringLiteral("light");
        case Variant::System: return QStringLiteral("system");
    }
    return QStringLiteral("dark");
}

Theme::Variant Theme::variantFromString(const QString& s)
{
    const QString id = normalizeId(s);
    if (id == kSystemThemeId) {
        return Variant::System;
    }
    if (id == kLightAmber) {
        return Variant::Light;
    }
    return Variant::Dark;
}

QString Theme::normalizeThemeId(const QString& id)
{
    return normalizeId(id);
}

QString Theme::defaultThemeId()
{
    return kDefaultThemeId;
}

void Theme::applyDefault()
{
    QSettings s;
    const QString persisted = normalizeId(s.value(kSettingsKey, defaultThemeId()).toString());
    applyTheme(persisted);
}

void Theme::applyTheme(const QString& id)
{
    const QString normalized = normalizeId(id);
    const ThemeRecord rec = themeForId(normalized);
    const QString appliedId = rec.id.isEmpty() ? defaultThemeId() : rec.id;

    currentThemeId_ = appliedId;
    current_ = variantFromString(appliedId);

    QSettings s;
    s.setValue(kSettingsKey, appliedId);

    const QString fontName = firstFontFamily(rec.fontFamily);
    if (!fontName.isEmpty()) {
        QFont f(fontName);
        f.setStyleHint(QFont::SansSerif, QFont::PreferAntialias);
        QApplication::setFont(f);
    }

    if (auto* fcApp = Gui::Application::Instance) {
        try {
            fcApp->reloadStyleSheet();
            refreshDockChromeTint();
            return;
        }
        catch (...) {
            // Fall through to direct apply if reload isn't available yet.
        }
    }

    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setStyleSheet(stylesheetForId(appliedId));
        refreshDockChromeTint();
    }
}

void Theme::applyVariant(Variant v)
{
    switch (v) {
        case Variant::System:
            applyTheme(kSystemThemeId);
            return;
        case Variant::Light:
            applyTheme(kLightAmber);
            return;
        case Variant::Dark:
        default:
            applyTheme(kDefaultThemeId);
            return;
    }
}

QString Theme::loadStylesheet(Variant v) const
{
    return stylesheetFor(v);
}

QString Theme::stylesheetFor(Variant v)
{
    switch (v) {
        case Variant::System:
            return QString();
        case Variant::Light:
            return stylesheetForId(kLightAmber);
        case Variant::Dark:
        default:
            return stylesheetForId(kDefaultThemeId);
    }
}

QList<Theme::ThemeRecord> Theme::discoveredThemes()
{
    QList<ThemeRecord> themes = discoverThemes();
    if (themes.isEmpty()) {
        themes.append(fallbackRecord());
    }
    return themes;
}

Theme::ThemeRecord Theme::fallbackRecord()
{
    ThemeRecord rec;
    rec.id = kDefaultThemeId;
    rec.name = QStringLiteral("Charcoal Amber");
    rec.author = QStringLiteral("LinuxCAD");
    rec.description = QStringLiteral("Fallback theme when manifests are unavailable.");
    rec.sourcePath = QStringLiteral(":/linuxcad/themes/charcoal-amber.theme.toml");
    rec.colors.insert(QStringLiteral("bg_primary"), QStringLiteral("#14171C"));
    rec.colors.insert(QStringLiteral("bg_secondary"), QStringLiteral("#1B1F25"));
    rec.colors.insert(QStringLiteral("bg_hover"), QStringLiteral("#232830"));
    rec.colors.insert(QStringLiteral("border_subtle"), QStringLiteral("#2C323B"));
    rec.colors.insert(QStringLiteral("border_strong"), QStringLiteral("#3A4150"));
    rec.colors.insert(QStringLiteral("text_primary"), QStringLiteral("#ECEEF1"));
    rec.colors.insert(QStringLiteral("text_muted"), QStringLiteral("#A8AEB8"));
    rec.colors.insert(QStringLiteral("text_dim"), QStringLiteral("#6B7280"));
    rec.colors.insert(QStringLiteral("accent"), QStringLiteral("#F59E0B"));
    rec.colors.insert(QStringLiteral("accent_hover"), QStringLiteral("#FFB933"));
    rec.colors.insert(QStringLiteral("accent_pressed"), QStringLiteral("#B36F00"));
    rec.colors.insert(QStringLiteral("scrollbar_bg"), QStringLiteral("#14171C"));
    rec.colors.insert(QStringLiteral("scrollbar_thumb"), QStringLiteral("#3A4150"));
    rec.colors.insert(QStringLiteral("selection_bg"), QStringLiteral("#232830"));
    rec.colors.insert(QStringLiteral("selection_text"), QStringLiteral("#ECEEF1"));
    rec.colors.insert(QStringLiteral("ai_idle"), QStringLiteral("#F59E0B"));
    rec.colors.insert(QStringLiteral("ai_thinking"), QStringLiteral("#FFB933"));
    rec.colors.insert(QStringLiteral("ai_error"), QStringLiteral("#F08080"));
    rec.colors.insert(QStringLiteral("ai_disabled"), QStringLiteral("#6B7280"));
    return rec;
}

Theme::ThemeRecord Theme::themeForId(const QString& id)
{
    const QString normalized = normalizeId(id);
    const QList<ThemeRecord> themes = discoveredThemes();
    for (const ThemeRecord& rec : themes) {
        if (rec.id.compare(normalized, Qt::CaseInsensitive) == 0) {
            return rec;
        }
    }
    for (const ThemeRecord& rec : themes) {
        if (rec.id.compare(defaultThemeId(), Qt::CaseInsensitive) == 0) {
            return rec;
        }
    }
    return fallbackRecord();
}

QString Theme::stylesheetForId(const QString& id)
{
    const QString normalized = normalizeId(id);
    const ThemeRecord rec = themeForId(normalized);
    if (rec.id.compare(kSystemThemeId, Qt::CaseInsensitive) == 0) {
        return QString();
    }

    // Use full polished theme QSS for bundled dark/light so all selectors
    // (including typography/readability fixes) are applied.
    if (rec.id.compare(kDefaultThemeId, Qt::CaseInsensitive) == 0
        || rec.id.compare(kLightAmber, Qt::CaseInsensitive) == 0) {
        QString out = readTextFile(legacyQssPathForId(rec.id));
        if (!rec.extraQss.isEmpty()) {
            out += QStringLiteral("\n") + rec.extraQss;
        }
        return out;
    }

    const QHash<QString, QString> fallbackColors = themeForId(defaultThemeId()).colors;
    const QString templ = readTextFile(kBaseTemplatePath);
    if (templ.isEmpty()) {
        return readTextFile(legacyQssPathForId(rec.id));
    }

    QString out = substituteTokens(templ, rec.colors, fallbackColors);
    if (!rec.extraQss.isEmpty()) {
        out += QStringLiteral("\n") + rec.extraQss;
    }
    return out;
}

QString Theme::currentStylesheet()
{
    QSettings s;
    const QString persisted = normalizeId(s.value(kSettingsKey, defaultThemeId()).toString());
    return stylesheetForId(persisted);
}

} // namespace LinuxCAD
} // namespace Gui
