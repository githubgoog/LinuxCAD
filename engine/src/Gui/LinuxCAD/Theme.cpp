// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QTextStream>
#endif

#include <Gui/Application.h>

#include "Theme.h"

namespace Gui {
namespace LinuxCAD {

namespace {
const QString kSettingsKey = QStringLiteral("LinuxCAD/Theme");

QString resourcePathFor(Theme::Variant v)
{
    switch (v) {
        case Theme::Variant::Dark:   return QStringLiteral(":/linuxcad/themes/dark.qss");
        case Theme::Variant::Light:  return QStringLiteral(":/linuxcad/themes/light.qss");
        case Theme::Variant::System: return QString();
    }
    return QString();
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
    if (s.compare(QStringLiteral("light"), Qt::CaseInsensitive) == 0)  return Variant::Light;
    if (s.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0) return Variant::System;
    return Variant::Dark;
}

void Theme::applyDefault()
{
    QSettings s;
    const QString persisted = s.value(kSettingsKey, QStringLiteral("dark")).toString();
    applyVariant(variantFromString(persisted));
}

void Theme::applyVariant(Variant v)
{
    current_ = v;

    QSettings s;
    s.setValue(kSettingsKey, variantToString(v));

    // Defer to FreeCAD's stylesheet pipeline. Application::setStyleSheet now
    // appends Theme::currentStylesheet() at the end, so a single reload picks
    // up our QSS atomically with whatever FreeCAD theme is active.
    if (auto* fcApp = Gui::Application::Instance) {
        try {
            fcApp->reloadStyleSheet();
            return;
        }
        catch (...) {
            // Fall through to direct apply if reload isn't available yet.
        }
    }

    // Fallback - used during very early startup before MainWindow exists.
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setStyleSheet(v == Variant::System ? QString() : loadStylesheet(v));
    }
}

QString Theme::loadStylesheet(Variant v) const
{
    return stylesheetFor(v);
}

QString Theme::stylesheetFor(Variant v)
{
    const QString path = resourcePathFor(v);
    if (path.isEmpty()) {
        return QString();
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

QString Theme::currentStylesheet()
{
    QSettings s;
    const QString persisted = s.value(kSettingsKey, QStringLiteral("dark")).toString();
    return stylesheetFor(variantFromString(persisted));
}

} // namespace LinuxCAD
} // namespace Gui
