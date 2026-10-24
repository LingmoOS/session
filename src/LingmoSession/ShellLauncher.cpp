// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "private/ShellLauncher_p.h"

#include <LingmoSession/ShellChannel.h>
#include <LingmoSession/ShellLauncher.h>

#include <QLoggingCategory>
#include <QProcess>
#include <QProcessEnvironment>

#include <fcntl.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(lcShell, "lingmo.session.shell")

namespace Lingmo {

ShellLauncher::ShellLauncher(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ShellLauncherPrivate>())
{
}

ShellLauncher::~ShellLauncher()
{
    stop();
}

bool ShellLauncher::launch(const QString &program,
                           const QString &waylandSocket,
                           ShellChannel *channel)
{
    if (d->process && d->process->state() != QProcess::NotRunning)
        return false;

    d->program = program;

    int shellFd = -1;
    if (channel && channel->isConnected())
    {
        shellFd = channel->createShellConnection();
        if (shellFd < 0)
            qCWarning(lcShell) << "No shell connection; desktop will be unavailable";
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("WAYLAND_DISPLAY"), waylandSocket);
    env.remove(QStringLiteral("WAYLAND_SOCKET"));

    QStringList args;
    args << QStringLiteral("--daemon");
    if (shellFd >= 0)
        args << QStringLiteral("--desktop-socket") << QString::number(shellFd);

    d->process = new QProcess(this);
    d->process->setProcessChannelMode(QProcess::ForwardedChannels);
    d->process->setProcessEnvironment(env);

    if (shellFd >= 0)
    {
        // Clear close-on-exec on shellFd inside the child so the shell inherits it.
        d->process->setChildProcessModifier([shellFd]()
        {
            ::fcntl(shellFd, F_SETFD, 0);
        });
    }

    QObject::connect(d->process,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     this,
                     [this](int code, QProcess::ExitStatus status)
    {
        qCInfo(lcShell) << d->program << "exited:" << code
                        << (status == QProcess::CrashExit ? "(crashed)" : "(normal)");
        emit finished(code, status == QProcess::CrashExit);
    });

    d->process->start(program, args);

    // Parent no longer needs shellFd after the child has it.
    if (shellFd >= 0)
        ::close(shellFd);

    if (d->process->state() == QProcess::NotRunning)
    {
        qCWarning(lcShell) << program << "failed to start";
        return false;
    }

    qCInfo(lcShell) << "Started" << program << "PID" << d->process->processId();
    return true;
}

void ShellLauncher::stop()
{
    if (!d->process || d->process->state() == QProcess::NotRunning)
        return;

    d->process->terminate();
    if (!d->process->waitForFinished(3000))
    {
        d->process->kill();
        d->process->waitForFinished(1000);
    }
}

bool ShellLauncher::isRunning() const
{
    return d->process && d->process->state() == QProcess::Running;
}

} // namespace Lingmo
