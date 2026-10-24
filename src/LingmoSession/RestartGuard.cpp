// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "private/RestartGuard_p.h"

#include <LingmoSession/RestartGuard.h>

#include <QLoggingCategory>
#include <QTimer>

Q_LOGGING_CATEGORY(lcRestart, "lingmo.session.restart")

namespace Lingmo {

RestartGuard::RestartGuard(const QString &name,
                           const Policy &policy,
                           QObject *parent)
    : QObject(parent)
    , d(std::make_unique<RestartGuardPrivate>())
{
    d->name   = name;
    d->policy = policy;

    d->retryTimer = new QTimer(this);
    d->retryTimer->setSingleShot(true);
    QObject::connect(d->retryTimer, &QTimer::timeout, this,
                     [this]()
    {
        if (!d->cancelled)
            emit shouldRestart();
    });
}

RestartGuard::~RestartGuard() = default;

void RestartGuard::notifyStarted()
{
    d->uptimeTimer.start();
}

void RestartGuard::notifyExited(int exitCode, bool crashed)
{
    if (d->cancelled || d->retryTimer->isActive())
        return;

    // A process that survived the stable-uptime window gets a fresh budget.
    if (d->uptimeTimer.isValid()
        && d->uptimeTimer.elapsed() >= d->policy.stableUptimeMs)
    {
        d->restarts = 0;
    }

    if (d->restarts >= d->policy.maxRestarts)
    {
        qCCritical(lcRestart) << d->name << "failed" << d->restarts
                              << "times; giving up";
        d->gaveUp = true;
        emit gaveUp();
        return;
    }

    const int delay = d->policy.initialDelayMs
                    + d->restarts * d->policy.backoffMs;
    qCWarning(lcRestart) << d->name << "exited" << exitCode
                         << (crashed ? "(crashed)" : "(normal)")
                         << "– restarting in" << delay << "ms"
                         << '(' << d->restarts + 1 << '/' << d->policy.maxRestarts << ')';
    ++d->restarts;
    d->retryTimer->start(delay);
}

void RestartGuard::cancel()
{
    d->cancelled = true;
    d->retryTimer->stop();
}

bool RestartGuard::isPending() const
{
    return d->retryTimer->isActive();
}

bool RestartGuard::hasGivenUp() const
{
    return d->gaveUp;
}

} // namespace Lingmo
