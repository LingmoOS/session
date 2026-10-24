// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <LingmoSession/EnvironmentSetup.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QLoggingCategory>
#include <QProcess>
#include <QStandardPaths>

#include <cstdlib>

Q_LOGGING_CATEGORY(lcEnv, "lingmo.session.env")

namespace Lingmo {

static void setenvIfEmpty(const char *name, const QString &value)
{
    if (qgetenv(name).isEmpty())
        qputenv(name, value.toUtf8());
}

EnvironmentSetup::EnvironmentSetup(QObject *parent)
    : QObject(parent)
{
}

EnvironmentSetup::~EnvironmentSetup() = default;

bool EnvironmentSetup::setup()
{
    if (!ensureXdgDirs())
        return false;
    if (!createRuntimeDirs())
        return false;
    setEnvironmentVariables();
    syncDbusEnvironment();
    importSystemdEnvironment();
    return true;
}

bool EnvironmentSetup::createRuntimeDirs()
{
    const QString runtimeDir =
        QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
        + QStringLiteral("/lingmo");
    if (!QDir().mkpath(runtimeDir))
    {
        qCWarning(lcEnv) << "Cannot create runtime directory:" << runtimeDir;
        return false;
    }
    return true;
}

void EnvironmentSetup::setEnvironmentVariables()
{
    const QString home = QDir::homePath();

    // XDG base directories — only set when not already in the environment.
    setenvIfEmpty("XDG_DATA_HOME",   home + QStringLiteral("/.local/share"));
    setenvIfEmpty("XDG_CONFIG_HOME", home + QStringLiteral("/.config"));
    setenvIfEmpty("XDG_CACHE_HOME",  home + QStringLiteral("/.cache"));
    setenvIfEmpty("XDG_DATA_DIRS",   QStringLiteral("/usr/local/share:/usr/share"));
    setenvIfEmpty("XDG_CONFIG_DIRS", QStringLiteral("/etc/xdg"));

    // Desktop identity.
    qputenv("DESKTOP_SESSION",    "Lingmo");
    qputenv("XDG_CURRENT_DESKTOP","Lingmo");
    qputenv("XDG_SESSION_DESKTOP","Lingmo");
    qputenv("XDG_SESSION_TYPE",   "wayland");

    // Qt / GTK platform.
    qputenv("QT_QPA_PLATFORM",       "wayland");
    qputenv("GDK_BACKEND",           "wayland");
    qputenv("QT_QPA_PLATFORMTHEME",  "lingmo");
    qputenv("QT_QPA_UPDATE_IDLE_TIME","10");

    // GSettings: use the plain-text keyfile backend — no dconf daemon needed.
    qputenv("GSETTINGS_BACKEND", "keyfile");

    // Wayland display socket name.
    setenvIfEmpty("WAYLAND_DISPLAY", "lingmo-0");

    // Remove input-method module overrides; the compositor's text-input
    // protocols should be used instead (XMODIFIERS stays for Xwayland).
    qunsetenv("GTK_IM_MODULE");
    qunsetenv("QT_IM_MODULE");

    qCInfo(lcEnv) << "Session environment configured";
}

bool EnvironmentSetup::ensureXdgDirs()
{
    const QStringList dirs = {
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation),
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation),
    };
    for (const QString &dir : dirs)
    {
        if (!QDir().mkpath(dir))
        {
            qCWarning(lcEnv) << "Cannot create XDG directory:" << dir;
            return false;
        }
    }
    return true;
}

void EnvironmentSetup::syncDbusEnvironment()
{
    // Push the current environment into D-Bus activation and systemd user scope.
    const int rc = QProcess::execute(
        QStringLiteral("dbus-update-activation-environment"),
        {QStringLiteral("--systemd"), QStringLiteral("--all")});
    if (rc != 0)
        qCWarning(lcEnv) << "dbus-update-activation-environment exited with" << rc;

    // Unset input-method variables from systemd's copy too (they may linger
    // from a previous session).
    QProcess::execute(
        QStringLiteral("systemctl"),
        {QStringLiteral("--user"), QStringLiteral("unset-environment"),
         QStringLiteral("GTK_IM_MODULE"),
         QStringLiteral("QT_IM_MODULE"),
         QStringLiteral("LC_ALL")});
}

void EnvironmentSetup::importSystemdEnvironment()
{
    QDBusInterface mgr(
        QStringLiteral("org.freedesktop.systemd1"),
        QStringLiteral("/org/freedesktop/systemd1"),
        QStringLiteral("org.freedesktop.systemd1.Manager"),
        QDBusConnection::sessionBus());

    if (!mgr.isValid())
        return;

    const QDBusReply<QStringList> reply =
        mgr.property("Environment").toStringList();
    // Use the raw variant path since property() returns QVariant.
    const QVariant envVar = mgr.property("Environment");
    if (!envVar.isValid())
        return;

    const QStringList entries = envVar.toStringList();
    static const QStringList kSkip = {
        QStringLiteral("_"),
        QStringLiteral("SHLVL"),
        QStringLiteral("DISPLAY"),
        QStringLiteral("XAUTHORITY"),
        QStringLiteral("WAYLAND_DISPLAY"),
        QStringLiteral("WAYLAND_SOCKET"),
        QStringLiteral("GTK_IM_MODULE"),
        QStringLiteral("QT_IM_MODULE"),
        QStringLiteral("LC_ALL"),
    };

    for (const QString &entry : entries)
    {
        const int eq = entry.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString name  = entry.left(eq);
        const QString value = entry.mid(eq + 1);

        bool skip = false;
        for (const QString &s : kSkip)
        {
            if (name == s || name.startsWith(s + QLatin1Char('_')))
            {
                skip = true;
                break;
            }
        }
        if (skip || name.startsWith(QStringLiteral("XDG_")))
            continue;

        const QByteArray current = qgetenv(name.toUtf8().constData());
        if (current != value.toUtf8())
            qputenv(name.toUtf8().constData(), value.toUtf8());
    }
}

} // namespace Lingmo
