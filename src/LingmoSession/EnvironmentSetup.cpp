#include <LingmoSession/EnvironmentSetup.h>
#include <QDir>
#include <QStandardPaths>
#include <QProcessEnvironment>
namespace Lingmo {
EnvironmentSetup::EnvironmentSetup(QObject *parent) : QObject(parent) {}
EnvironmentSetup::~EnvironmentSetup() = default;
bool EnvironmentSetup::setup() {
    if (!createRuntimeDirs()) return false;
    setEnvironmentVariables();
    return true;
}
bool EnvironmentSetup::createRuntimeDirs() {
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
        + QStringLiteral("/lingmo");
    return QDir().mkpath(runtimeDir);
}
void EnvironmentSetup::setEnvironmentVariables() {
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/lingmo");
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/lingmo");
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/lingmo");
    qputenv("LINGMO_CONFIG_DIR", configDir.toUtf8());
    qputenv("LINGMO_DATA_DIR", dataDir.toUtf8());
    qputenv("LINGMO_CACHE_DIR", cacheDir.toUtf8());
    qputenv("LINGMO_VERSION", "1.0.0");
    qputenv("XDG_CURRENT_DESKTOP", "Lingmo");
    qputenv("XDG_SESSION_TYPE", "wayland");
}
bool EnvironmentSetup::ensureXdgDirs() {
    const QStringList dirs = {
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation),
        QStandardPaths::writableLocation(QStandardPaths::StateLocation),
    };
    for (const QString &dir : dirs) {
        if (!QDir().mkpath(dir))
            return false;
    }
    return true;
}
} // namespace Lingmo
