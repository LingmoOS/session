// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "private/SessionManager_p.h"

#include <LingmoSession/Component.h>
#include <LingmoSession/CrashRecovery.h>
#include <LingmoSession/RestartGuard.h>
#include <LingmoSession/SessionManager.h>

#include <QDBusConnection>
#include <QLoggingCategory>
#include <QProcess>
#include <QTimer>

Q_LOGGING_CATEGORY(lcSession, "lingmo.session")

namespace Lingmo {

static SessionManager *s_instance = nullptr;

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<SessionManagerPrivate>())
{
    d->crashRecovery = std::make_unique<CrashRecovery>(this);
    d->restartTimer  = new QTimer(this);
    d->restartTimer->setSingleShot(true);

    QObject::connect(d->restartTimer, &QTimer::timeout, this,
                     [this]()
    {
        for (auto it = d->components.constBegin();
             it != d->components.constEnd(); ++it)
        {
            if (it.value()->state() == ComponentState::Failed)
                restartComponent(it.key());
        }
    });
}

SessionManager::~SessionManager() = default;

SessionManager *SessionManager::instance()
{
    if (!s_instance)
        s_instance = new SessionManager;
    return s_instance;
}

Lingmo::SessionState SessionManager::state() const
{
    return d->state;
}

void SessionManager::registerComponent(Component *component)
{
    if (!component)
        return;

    d->components.insert(component->name(), component);

    QObject::connect(component, &Component::stateChanged, this,
                     [this, name = component->name()](ComponentState state)
    {
        if (state == ComponentState::Ready
            || state == ComponentState::Running)
        {
            if (!d->readyComponents.contains(name))
            {
                d->readyComponents.append(name);
                emit componentStarted(name);
                emit componentsRunningChanged();
                tryStartPending();
            }
        }
        else if (state == ComponentState::Failed)
        {
            emit componentFailed(name, QStringLiteral("Component failed"));
            d->restartTimer->start(d->restartDelayMs);
        }
    });
}

bool SessionManager::startSession()
{
    if (d->state == SessionState::Running
        || d->state == SessionState::Starting)
        return false;

    // Register the session on D-Bus so shell/services can call Logout etc.
    QDBusConnection::sessionBus().registerService(
        QStringLiteral("org.lingmo.Session"));

    d->state = SessionState::Starting;
    emit stateChanged(d->state);
    d->readyComponents.clear();
    tryStartPending();
    return true;
}

void SessionManager::tryStartPending()
{
    bool allReady = true;

    for (auto it = d->components.constBegin();
         it != d->components.constEnd(); ++it)
    {
        Component *comp = it.value();
        const QString name = it.key();

        if (comp->state() != ComponentState::Stopped
            && comp->state() != ComponentState::Failed)
            continue;

        if (!comp->dependenciesSatisfied(d->readyComponents))
        {
            allReady = false;
            continue;
        }

        qCInfo(lcSession) << "Starting component:" << name;
        comp->setState(ComponentState::Starting);

        // Launch via QProcess so we can monitor exit.
        auto *proc = new QProcess(this);
        proc->setProcessChannelMode(QProcess::ForwardedChannels);
        d->processes.insert(name, proc);

        // Restart guard for this component.
        RestartGuard::Policy policy;
        policy.maxRestarts = d->maxRestarts;
        auto *guard = new RestartGuard(name, policy, this);
        d->guards.insert(name, guard);

        QObject::connect(guard, &RestartGuard::shouldRestart, this,
                         [this, name]() { restartComponent(name); });
        QObject::connect(guard, &RestartGuard::gaveUp, this,
                         [this, name]()
        {
            qCCritical(lcSession) << "Component gave up after repeated crashes:" << name;
            if (auto *c = d->components.value(name))
                c->setState(ComponentState::Failed);
            emit componentFailed(name, QStringLiteral("Crash limit exceeded"));
        });

        QObject::connect(proc,
                         QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         this,
                         [this, name, guard](int code, QProcess::ExitStatus status)
        {
            const bool crashed = (status == QProcess::CrashExit);
            qCInfo(lcSession) << name << "exited:" << code
                              << (crashed ? "(crashed)" : "(normal)");
            if (auto *c = d->components.value(name))
                c->setState(crashed ? ComponentState::Failed : ComponentState::Stopped);
            guard->notifyExited(code, crashed);
        });

        const QStringList parts =
            comp->command().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        const QString prog = parts.isEmpty() ? comp->command() : parts.first();
        const QStringList args = parts.mid(1);

        guard->notifyStarted();
        proc->start(prog, args);

        if (proc->state() == QProcess::NotRunning)
        {
            qCWarning(lcSession) << "Failed to start component:" << name;
            comp->setState(ComponentState::Failed);
            allReady = false;
        }
        else
        {
            comp->setState(ComponentState::Running);
        }
    }

    if (allReady && d->state == SessionState::Starting)
    {
        d->state = SessionState::Running;
        emit stateChanged(d->state);
        emit sessionReady();
    }
}

void SessionManager::stopSession()
{
    if (d->state == SessionState::Stopping)
        return;

    d->state = SessionState::Stopping;
    emit stateChanged(d->state);

    // Cancel all restart guards first.
    for (auto *guard : std::as_const(d->guards))
        guard->cancel();

    // Stop processes in reverse registration order.
    const QStringList names = d->components.keys();
    for (int i = names.size() - 1; i >= 0; --i)
    {
        const QString &name = names.at(i);
        Component *comp = d->components.value(name);
        QProcess  *proc = d->processes.value(name);

        if (proc && proc->state() != QProcess::NotRunning)
        {
            proc->terminate();
            if (!proc->waitForFinished(3000))
                proc->kill();
        }
        if (comp->state() == ComponentState::Running)
        {
            comp->setState(ComponentState::Stopped);
            emit componentStopped(name);
        }
    }

    d->state = SessionState::Idle;
    emit stateChanged(d->state);
    emit sessionStopped();
}

bool SessionManager::restartComponent(const QString &name)
{
    auto *comp = d->components.value(name);
    if (!comp)
        return false;

    if (d->crashRecovery->isCrashLimitExceeded(name))
    {
        qCWarning(lcSession) << "Crash limit exceeded for" << name;
        emit componentFailed(name, QStringLiteral("Crash limit exceeded"));
        return false;
    }

    d->crashRecovery->recordCrash(name);
    comp->setState(ComponentState::Stopped);
    qCInfo(lcSession) << "Restarting component:" << name;

    // Remove old process.
    if (auto *old = d->processes.take(name))
    {
        old->deleteLater();
    }

    // Re-use tryStartPending logic by marking stopped + retriggering.
    tryStartPending();
    return true;
}

Component *SessionManager::component(const QString &name) const
{
    return d->components.value(name);
}

QStringList SessionManager::componentNames() const
{
    return d->components.keys();
}

int SessionManager::componentsRunning() const
{
    int count = 0;
    for (auto it = d->components.constBegin();
         it != d->components.constEnd(); ++it)
    {
        if (it.value()->state() == ComponentState::Running)
            ++count;
    }
    return count;
}

void SessionManager::setMaxRestarts(int max)  { d->maxRestarts    = max; }
void SessionManager::setRestartDelay(int ms)  { d->restartDelayMs = ms;  }

} // namespace Lingmo
