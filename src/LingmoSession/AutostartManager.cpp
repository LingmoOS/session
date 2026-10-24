// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "private/AutostartManager_p.h"

#include <LingmoSession/AutostartManager.h>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(lcAutostart, "lingmo.session.autostart")

namespace Lingmo {

// Programs that the session itself starts; autostart entries must not
// duplicate them.
static const QStringList kSessionPrograms = {
    QStringLiteral("lingmo-services"),
    QStringLiteral("lingmo-compositor"),
    QStringLiteral("lingmo-polkit-agent"),
    QStringLiteral("lingmo-shell-core"),
};

static bool isSessionManaged(const QString &execLine)
{
    const QString prog = execLine.section(QLatin1Char(' '), 0, 0);
    const QString base = QFileInfo(prog).fileName();
    return kSessionPrograms.contains(base);
}

// Remove XDG Desktop Entry field codes (%u, %U, %f, %F, etc.) from the
// command string — autostart entries take no file arguments.
static QStringList parseExec(const QString &execLine)
{
    QStringList result;
    bool inQuote = false;
    QString current;

    const int len = execLine.length();
    for (int i = 0; i < len; ++i)
    {
        const QChar c = execLine.at(i);
        if (c == QLatin1Char('"'))
        {
            inQuote = !inQuote;
            continue;
        }
        if (!inQuote && c == QLatin1Char(' '))
        {
            if (!current.isEmpty())
            {
                result << current;
                current.clear();
            }
            continue;
        }
        if (c == QLatin1Char('%') && i + 1 < len)
        {
            ++i; // Skip the specifier character.
            continue;
        }
        current += c;
    }
    if (!current.isEmpty())
        result << current;

    // Drop any remaining bare field codes (e.g. "%U" that survived as tokens).
    result.erase(
        std::remove_if(result.begin(), result.end(),
                       [](const QString &s)
                       { return s.length() == 2 && s.at(0) == QLatin1Char('%'); }),
        result.end());

    return result;
}

// AutostartManager -----------------------------------------------------------

AutostartManager::AutostartManager(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<AutostartManagerPrivate>())
{
    // User autostart directory has higher precedence than system-wide.
    d->searchPaths
        << QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QStringLiteral("/autostart")
        << QStringLiteral("/etc/xdg/autostart");
}

AutostartManager::~AutostartManager() = default;

void AutostartManager::discover()
{
    d->discoveredApps.clear();

    // Track file names that have been seen so higher-precedence paths win.
    QSet<QString> seen;
    const QString currentDesktop =
        QString::fromUtf8(qgetenv("XDG_CURRENT_DESKTOP"));
    const QStringList desktops =
        currentDesktop.split(QLatin1Char(':'), Qt::SkipEmptyParts);

    for (const QString &searchPath : std::as_const(d->searchPaths))
    {
        const QDir dir(searchPath);
        if (!dir.exists())
            continue;

        const QStringList entries =
            dir.entryList({QStringLiteral("*.desktop")}, QDir::Files);

        for (const QString &fileName : entries)
        {
            if (seen.contains(fileName))
                continue;
            seen.insert(fileName);

            const QString filePath = dir.absoluteFilePath(fileName);
            QSettings desktop(filePath, QSettings::IniFormat);
            desktop.beginGroup(QStringLiteral("Desktop Entry"));

            // Skip hidden / disabled entries.
            if (desktop.value(QStringLiteral("Hidden"), false).toBool())
                continue;
            if (desktop.value(QStringLiteral("NoDisplay"), false).toBool())
                continue;
            if (desktop.value(QStringLiteral("Type")).toString()
                != QStringLiteral("Application"))
                continue;

            // OnlyShowIn / NotShowIn filtering.
            const QString onlyShowIn =
                desktop.value(QStringLiteral("OnlyShowIn")).toString();
            if (!onlyShowIn.isEmpty())
            {
                const QStringList allowed =
                    onlyShowIn.split(QLatin1Char(';'), Qt::SkipEmptyParts);
                bool match = false;
                for (const QString &d : desktops)
                {
                    if (allowed.contains(d))
                    {
                        match = true;
                        break;
                    }
                }
                if (!match)
                    continue;
            }

            const QString notShowIn =
                desktop.value(QStringLiteral("NotShowIn")).toString();
            if (!notShowIn.isEmpty())
            {
                const QStringList blocked =
                    notShowIn.split(QLatin1Char(';'), Qt::SkipEmptyParts);
                bool blocked_ = false;
                for (const QString &d : desktops)
                {
                    if (blocked.contains(d))
                    {
                        blocked_ = true;
                        break;
                    }
                }
                if (blocked_)
                    continue;
            }

            // TryExec: skip if the binary is not available.
            const QString tryExec =
                desktop.value(QStringLiteral("TryExec")).toString().trimmed();
            if (!tryExec.isEmpty())
            {
                const bool absolute = tryExec.startsWith(QLatin1Char('/'));
                const bool exists   = absolute
                    ? QFileInfo::exists(tryExec)
                    : !QStandardPaths::findExecutable(tryExec).isEmpty();
                if (!exists)
                    continue;
            }

            const QString exec =
                desktop.value(QStringLiteral("Exec")).toString().trimmed();
            if (exec.isEmpty())
                continue;

            if (isSessionManaged(exec))
                continue;

            d->discoveredApps << filePath;
        }
    }

    qCInfo(lcAutostart) << "Discovered" << d->discoveredApps.size()
                        << "autostart entries";
}

void AutostartManager::launchAll()
{
    for (const QString &filePath : std::as_const(d->discoveredApps))
    {
        QSettings desktop(filePath, QSettings::IniFormat);
        desktop.beginGroup(QStringLiteral("Desktop Entry"));

        const QString name =
            desktop.value(QStringLiteral("Name")).toString();
        const QString exec =
            desktop.value(QStringLiteral("Exec")).toString().trimmed();

        const QStringList command = parseExec(exec);
        if (command.isEmpty())
            continue;

        const QString program = command.first();
        const QStringList args = command.mid(1);

        const bool ok = QProcess::startDetached(program, args);
        if (ok)
        {
            qCInfo(lcAutostart) << "Launched autostart:" << name;
            emit applicationLaunched(name);
        }
        else
        {
            qCWarning(lcAutostart) << "Failed to launch autostart:" << name;
            emit applicationFailed(name, QStringLiteral("Process could not be started"));
        }
    }
}

QStringList AutostartManager::applications() const
{
    return d->discoveredApps;
}

} // namespace Lingmo
