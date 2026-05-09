// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QChar>
#include <QStringList>
#endif

#include "MiniToml.h"

namespace Gui {
namespace LinuxCAD {

namespace {

QString trimmed(const QString& s)
{
    return s.trimmed();
}

QString stripInlineComment(const QString& s)
{
    // Inline comments are only honored when the '#' sits outside any
    // string. Since we only support quoted string values, a '#' inside
    // matched quotes must be preserved.
    bool inString = false;
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s.at(i);
        if (c == QLatin1Char('"')) {
            inString = !inString;
            continue;
        }
        if (!inString && c == QLatin1Char('#')) {
            return s.left(i);
        }
    }
    return s;
}

QString makeError(int line, const QString& reason)
{
    return QStringLiteral("line %1: %2").arg(line).arg(reason);
}

} // namespace

bool MiniToml::parse(const QString& text, QString* errorOut)
{
    data_.clear();

    const QStringList lines = text.split(QLatin1Char('\n'));
    QString currentSection;

    for (int idx = 0; idx < lines.size(); ++idx) {
        const int lineNo = idx + 1;
        QString line = lines.at(idx);

        // Strip a trailing CR for files written with CRLF line endings.
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }

        line = stripInlineComment(line);
        line = trimmed(line);

        if (line.isEmpty()) {
            continue;
        }

        // Section header: [name]
        if (line.startsWith(QLatin1Char('['))) {
            if (!line.endsWith(QLatin1Char(']'))) {
                if (errorOut) {
                    *errorOut = makeError(lineNo,
                        QStringLiteral("malformed section header"));
                }
                return false;
            }
            const QString name = line.mid(1, line.size() - 2).trimmed();
            if (name.isEmpty()) {
                if (errorOut) {
                    *errorOut = makeError(lineNo,
                        QStringLiteral("empty section header"));
                }
                return false;
            }
            if (name.contains(QLatin1Char('[')) || name.contains(QLatin1Char(']'))) {
                if (errorOut) {
                    *errorOut = makeError(lineNo,
                        QStringLiteral("nested section headers are not supported"));
                }
                return false;
            }
            currentSection = name;
            if (!data_.contains(currentSection)) {
                data_.insert(currentSection, QHash<QString, QString>());
            }
            continue;
        }

        // Key/value pair.
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq < 0) {
            if (errorOut) {
                *errorOut = makeError(lineNo,
                    QStringLiteral("expected '=' in key/value pair"));
            }
            return false;
        }

        const QString key = line.left(eq).trimmed();
        QString value = line.mid(eq + 1).trimmed();

        if (key.isEmpty()) {
            if (errorOut) {
                *errorOut = makeError(lineNo, QStringLiteral("empty key"));
            }
            return false;
        }
        if (currentSection.isEmpty()) {
            if (errorOut) {
                *errorOut = makeError(lineNo,
                    QStringLiteral("key '%1' outside of any [section]").arg(key));
            }
            return false;
        }

        // Reject obvious non-string scalars and structural types we do not
        // support: arrays, inline tables.
        if (value.startsWith(QLatin1Char('[')) || value.startsWith(QLatin1Char('{'))) {
            if (errorOut) {
                *errorOut = makeError(lineNo,
                    QStringLiteral("arrays and inline tables are not supported"));
            }
            return false;
        }

        // Require a quoted string value. An empty value (key = "") is fine
        // and represented as an empty string.
        if (!value.startsWith(QLatin1Char('"')) || !value.endsWith(QLatin1Char('"'))
            || value.size() < 2) {
            if (errorOut) {
                *errorOut = makeError(lineNo,
                    QStringLiteral("value for '%1' must be a double-quoted string").arg(key));
            }
            return false;
        }

        value = value.mid(1, value.size() - 2);

        // Reject embedded raw newlines: split() above guarantees we never
        // see '\n' here, but bail if anyone slips a literal CR through.
        if (value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r'))) {
            if (errorOut) {
                *errorOut = makeError(lineNo,
                    QStringLiteral("multi-line string values are not supported"));
            }
            return false;
        }

        data_[currentSection].insert(key, value);
    }

    return true;
}

QString MiniToml::getString(const QString& section,
                            const QString& key,
                            const QString& defaultValue) const
{
    const auto sectionIt = data_.constFind(section);
    if (sectionIt == data_.constEnd()) {
        return defaultValue;
    }
    const auto keyIt = sectionIt->constFind(key);
    if (keyIt == sectionIt->constEnd()) {
        return defaultValue;
    }
    return *keyIt;
}

QStringList MiniToml::keysIn(const QString& section) const
{
    const auto sectionIt = data_.constFind(section);
    if (sectionIt == data_.constEnd()) {
        return QStringList();
    }
    return sectionIt->keys();
}

bool MiniToml::hasSection(const QString& section) const
{
    return data_.contains(section);
}

} // namespace LinuxCAD
} // namespace Gui
