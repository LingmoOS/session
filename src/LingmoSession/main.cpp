// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <LingmoSession/AutostartManager.h>
#include <LingmoSession/CompositorLauncher.h>
#include <LingmoSession/Component.h>
#include <LingmoSession/EnvironmentSetup.h>
#include <LingmoSession/RestartGuard.h>
#include <LingmoSession/SessionManager.h>
#include <LingmoSession/ShellChannel.h>
#include <LingmoSession/ShellLauncher.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDir>
#include <QLoggingCategory>
#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(lcMain, "lingmo.session.main")

// ── Signal handling via self-pipe trick ──────────────────────────────────────

static int g_sigPipe[2] = {-1, -1};

static void signalHandler(int)
{
    const char byte = 1;
    [[maybe_unused]] ssize_t r = ::write(g_sigPipe[1], &byte, 1);
}

static void installSignalHandlers()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
                     0, g_sigPipe) != 0)
    {
        qCWarning(lcMain) << "Cannot install termination signal handlers";
        return;
    }
    struct sigaction sa {};
    sa.sa_handler = signalHandler;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    ::sigaction(SIGTERM, &sa, nullptr);
    ::sigaction(SIGINT,  &sa, nullptr);
    ::sigaction(SIGHUP,  &sa, nullptr);
}

// ── XDG user directories ─────────────────────────────────────────────────────

static void ensureXdgUserDirs()
{
    const QString home = QDir::homePath();
    static const char *const dirs[][2] = {
        {"Desktop",   "DESKTOP"},
        {"Downloads", "DOWNLOAD"},
        {"Templates", "TEMPLATES"},
        {"Public",    "PUBLICSHARE"},
        {"Documents", "DOCUMENTS"},
        {"Music",     "MUSIC"},
        {"Pictures",  "PICTURES"},
        {"Videos",    "VIDEOS"},
    };
    for (const auto &entry : dirs)
        QDir(home).mkpath(QString::fromUtf8(entry[0]));
}

// ── sd_notify helper ─────────────────────────────────────────────────────────

static void notifySystemdReady()
{
    const QByteArray notifySocket = qgetenv("NOTIFY_SOCKET");
    if (notifySocket.isEmpty())
        return;
    // Use systemd-notify binary — avoids linking libsystemd directly here.
    QProcess::startDetached(
        QStringLiteral("systemd-notify"),
        {QStringLiteral("--ready"),
         QStringLiteral("--status=Lingmo session running")});
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("lingmo-session"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("LingmoOS"));

    installSignalHandlers();

    qCInfo(lcMain) << "Lingmo Desktop session starting";

    // ── 1. Environment ──────────────────────────────────────────────────────
    Lingmo::EnvironmentSetup envSetup;
    if (!envSetup.setup())
    {
        qCCritical(lcMain) << "Environment setup failed";
        return 1;
    }

    ensureXdgUserDirs();

    // ── 2. D-Bus session bus sanity check ───────────────────────────────────
    if (!QDBusConnection::sessionBus().isConnected())
    {
        qCCritical(lcMain) << "Cannot connect to D-Bus session bus";
        return 1;
    }

    // ── 3. Session manager ──────────────────────────────────────────────────
    auto *session = Lingmo::SessionManager::instance();
    session->setMaxRestarts(5);
    session->setRestartDelay(2000);

    // ── 4. Compositor launcher + shell channel ──────────────────────────────
    auto *compositor = new Lingmo::CompositorLauncher(&app);
    auto *channel    = new Lingmo::ShellChannel(&app);
    auto *shell      = new Lingmo::ShellLauncher(&app);

    RestartGuard::Policy shellPolicy;
    shellPolicy.maxRestarts    = 5;
    shellPolicy.stableUptimeMs = 30'000;
    auto *shellGuard = new Lingmo::RestartGuard(
        QStringLiteral("lingmo-shell-core"), shellPolicy, &app);

    const QString waylandSocket =
        QString::fromUtf8(qgetenv("WAYLAND_DISPLAY").isEmpty()
                          ? "lingmo-0"
                          : qgetenv("WAYLAND_DISPLAY"));

    const QString runtimeDir =
        QString::fromUtf8(qgetenv("XDG_RUNTIME_DIR"));
    const QString controlSocket =
        runtimeDir + QLatin1Char('/') + waylandSocket
        + QStringLiteral("-shell.control");

    // Shell finished → tell guard, guard fires shouldRestart → relaunch.
    QObject::connect(shell, &Lingmo::ShellLauncher::finished,
                     &app,
                     [shellGuard](int code, bool crashed)
    {
        shellGuard->notifyExited(code, crashed);
    });

    QObject::connect(shellGuard, &Lingmo::RestartGuard::shouldRestart,
                     &app,
                     [shell, channel, waylandSocket]()
    {
        shell->launch(QStringLiteral("/usr/lib/lingmo/lingmo-shell-core"),
                      waylandSocket, channel);
    });

    // Compositor ready → connect channel, register components, start session.
    QObject::connect(compositor, &Lingmo::CompositorLauncher::ready,
                     &app,
                     [&]()
    {
        qCInfo(lcMain) << "Compositor ready";

        channel->connectToCompositor(controlSocket);

        // Core components — started in dependency order by SessionManager.
        session->registerComponent(
            new Lingmo::Component(
                QStringLiteral("lingmo-services"),
                QStringLiteral("/usr/lib/lingmo/lingmo-services"),
                {}, &app));

        session->registerComponent(
            new Lingmo::Component(
                QStringLiteral("lingmo-polkit-agent"),
                QStringLiteral("/usr/lib/lingmo/lingmo-polkit-agent"),
                {}, &app));

        session->startSession();

        // Shell starts after services are up (watches D-Bus name).
        shellGuard->notifyStarted();
        shell->launch(QStringLiteral("/usr/lib/lingmo/lingmo-shell-core"),
                      waylandSocket, channel);

        // Autostart — slight delay so the shell has time to initialise.
        QTimer::singleShot(1500, &app, []()
        {
            auto autostart = new Lingmo::AutostartManager;
            autostart->discover();
            autostart->launchAll();
            autostart->deleteLater();
        });

        notifySystemdReady();
    });

    QObject::connect(compositor, &Lingmo::CompositorLauncher::failed,
                     &app,
                     [](const QString &reason)
    {
        qCCritical(lcMain) << "Compositor failed:" << reason;
        QCoreApplication::exit(1);
    });

    QObject::connect(compositor, &Lingmo::CompositorLauncher::exited,
                     &app,
                     [](int code)
    {
        qCInfo(lcMain) << "Compositor exited with code" << code;
        QCoreApplication::exit(code);
    });

    // ── 5. Termination signal watcher ───────────────────────────────────────
    auto *sigNotifier =
        new QSocketNotifier(g_sigPipe[0], QSocketNotifier::Read, &app);
    QObject::connect(sigNotifier, &QSocketNotifier::activated, &app,
                     [session, compositor, shell]()
    {
        qCInfo(lcMain) << "Termination signal received — shutting down";
        shell->stop();
        session->stopSession();
        compositor->stop();
        QCoreApplication::exit(0);
    });

    // ── 6. Launch compositor ────────────────────────────────────────────────
    compositor->launch(
        QStringLiteral("/usr/lib/lingmo/lingmo-compositor"),
        waylandSocket,
        controlSocket);

    return QCoreApplication::exec();
}
