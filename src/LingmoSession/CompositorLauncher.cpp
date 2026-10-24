// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "private/CompositorLauncher_p.h"

#include <LingmoSession/CompositorLauncher.h>

#include <QLoggingCategory>
#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>

#include <fcntl.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(lcCompositor, "lingmo.session.compositor")

namespace Lingmo {

CompositorLauncher::CompositorLauncher(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<CompositorLauncherPrivate>())
{
}

CompositorLauncher::~CompositorLauncher()
{
    stop();
}

void CompositorLauncher::launch(const QString &binary,
                                const QString &socketName,
                                const QString &controlSocket,
                                int timeoutMs,
                                const QString &xwaylandReadyFile)
{
    if (d->process && d->process->state() != QProcess::NotRunning)
        return;

    d->ready    = false;
    d->stopping = false;

    // Create a pipe for the readiness signal. The write end is passed to the
    // compositor as an inherited fd; it writes 'R' once ready.
    int fds[2];
    if (::pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0)
    {
        emit failed(QStringLiteral("Cannot create compositor readiness pipe"));
        return;
    }
    d->readFd       = fds[0];
    const int writeFd = fds[1];

    // Build argument list.
    QStringList args;
    args << QStringLiteral("--wayland-socket") << socketName;
    args << QStringLiteral("--ready-fd")       << QString::number(writeFd);
    if (!controlSocket.isEmpty())
        args << QStringLiteral("--shell-control-socket") << controlSocket;
    if (!xwaylandReadyFile.isEmpty())
        args << QStringLiteral("--xwayland-ready-file") << xwaylandReadyFile;

    // Launch the compositor process.
    d->process = new QProcess(this);
    d->process->setProcessChannelMode(QProcess::ForwardedChannels);

    // Clear close-on-exec on the write end inside the child so the compositor
    // can write to it after exec().
    d->process->setChildProcessModifier([writeFd]()
    {
        ::fcntl(writeFd, F_SETFD, 0);
    });

    // Watch the read end for the readiness byte.
    d->readyNotifier = new QSocketNotifier(d->readFd, QSocketNotifier::Read, this);
    QObject::connect(d->readyNotifier, &QSocketNotifier::activated, this,
                     [this]()
    {
        char marker = 0;
        const ssize_t n = ::read(d->readFd, &marker, 1);
        if (n < 0)
            return;

        d->readyNotifier->setEnabled(false);
        d->timeoutTimer->stop();
        ::close(d->readFd);
        d->readFd = -1;

        if (n != 1 || marker != 'R')
        {
            emit failed(QStringLiteral("Compositor did not confirm startup"));
            return;
        }
        d->ready = true;
        emit ready();
    });

    // Startup timeout.
    d->timeoutTimer = new QTimer(this);
    d->timeoutTimer->setSingleShot(true);
    QObject::connect(d->timeoutTimer, &QTimer::timeout, this,
                     [this]()
    {
        if (!d->ready)
            emit failed(QStringLiteral("Compositor startup timed out"));
    });

    // Handle compositor exit.
    QObject::connect(d->process,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     this,
                     [this](int code, QProcess::ExitStatus status)
    {
        if (d->readFd >= 0)
        {
            ::close(d->readFd);
            d->readFd = -1;
        }
        if (d->stopping || !d->ready)
            return;
        emit exited(status == QProcess::CrashExit ? 1 : code);
    });

    d->process->start(binary, args);
    ::close(writeFd); // Parent does not need the write end.

    if (d->process->state() == QProcess::NotRunning)
    {
        emit failed(QStringLiteral("Cannot start ") + binary);
        return;
    }

    if (!d->ready)
        d->timeoutTimer->start(timeoutMs);
}

void CompositorLauncher::stop()
{
    d->stopping = true;

    if (d->readyNotifier)
    {
        d->readyNotifier->setEnabled(false);
        d->readyNotifier->deleteLater();
        d->readyNotifier = nullptr;
    }
    if (d->timeoutTimer)
    {
        d->timeoutTimer->stop();
        d->timeoutTimer->deleteLater();
        d->timeoutTimer = nullptr;
    }
    if (d->readFd >= 0)
    {
        ::close(d->readFd);
        d->readFd = -1;
    }
    if (d->process && d->process->state() != QProcess::NotRunning)
    {
        d->process->terminate();
        if (!d->process->waitForFinished(3000))
        {
            d->process->kill();
            d->process->waitForFinished(1000);
        }
    }
    d->ready = false;
}

bool CompositorLauncher::isReady() const
{
    return d->ready;
}

bool CompositorLauncher::isRunning() const
{
    return d->process && d->process->state() == QProcess::Running;
}

} // namespace Lingmo
