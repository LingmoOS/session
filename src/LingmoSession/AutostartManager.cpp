#include "private/AutostartManager_p.h"
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QProcess>
namespace Lingmo {
AutostartManager::AutostartManager(QObject *parent)
    : QObject(parent), d(std::make_unique<AutostartManagerPrivate>())
{
    d->searchPaths << QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                      + QStringLiteral("/autostart");
    d->searchPaths << QStringLiteral("/etc/xdg/autostart");
}
AutostartManager::~AutostartManager() = default;
void AutostartManager::discover() {
    d->discoveredApps.clear();
    for (const QString &path : d->searchPaths) {
        const QDir dir(path);
        if (!dir.exists()) continue;
        const QStringList entries = dir.entryList(
            QStringList() << QStringLiteral("*.desktop"), QDir::Files);
        for (const QString &entry : entries) {
            const QString filePath = dir.absoluteFilePath(entry);
            QSettings desktop(filePath, QSettings::IniFormat);
            desktop.beginGroup(QStringLiteral("Desktop Entry"));
            const bool hidden = desktop.value(QStringLiteral("Hidden"), false).toBool();
            const bool noDisplay = desktop.value(QStringLiteral("NoDisplay"), false).toBool();
            if (!hidden && !noDisplay) {
                d->discoveredApps.append(filePath);
            }
        }
    }
}
void AutostartManager::launchAll() {
    for (const QString &app : d->discoveredApps) {
        QSettings desktop(app, QSettings::IniFormat);
        desktop.beginGroup(QStringLiteral("Desktop Entry"));
        const QString exec = desktop.value(QStringLiteral("Exec")).toString();
        const QString name = desktop.value(QStringLiteral("Name")).toString();
        if (exec.isEmpty()) continue;
        bool ok = QProcess::startDetached(exec);
        if (ok)
            emit applicationLaunched(name);
        else
            emit applicationFailed(name, QStringLiteral("Failed to launch"));
    }
}
QStringList AutostartManager::applications() const { return d->discoveredApps; }
} // namespace Lingmo
