#include <LingmoSession/SessionManager.h>
#include <LingmoSession/EnvironmentSetup.h>
#include <LingmoSession/AutostartManager.h>
#include <LingmoSession/Component.h>
#include <LingmoIPC/ServiceManager.h>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("lingmo-session"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    qInfo("lingmo-session: Starting Lingmo Desktop session...");

    // Set up environment
    Lingmo::EnvironmentSetup envSetup;
    if (!envSetup.setup()) {
        qCritical("lingmo-session: Failed to set up environment");
        return 1;
    }

    // Notify systemd we're ready
    auto *svcMgr = Lingmo::ServiceManager::instance();
    svcMgr->setReady();

    // Create session manager
    auto *session = Lingmo::SessionManager::instance();

    // Register core components in dependency order
    session->registerComponent(new Lingmo::Component(
        QStringLiteral("settings-daemon"),
        QStringLiteral("/usr/libexec/lingmo-settings-daemon"),
        {}, &app));

    session->registerComponent(new Lingmo::Component(
        QStringLiteral("shell-core"),
        QStringLiteral("/usr/libexec/lingmo-shell-core"),
        {QStringLiteral("settings-daemon")}, &app));

    session->registerComponent(new Lingmo::Component(
        QStringLiteral("notification"),
        QStringLiteral("/usr/libexec/lingmo-notification"),
        {}, &app));

    // Launch autostart applications
    Lingmo::AutostartManager autostart;
    autostart.discover();
    autostart.launchAll();

    // Start the session
    session->startSession();

    qInfo("lingmo-session: Session started successfully");

    return app.exec();
}
