#include "private/SessionManager_p.h"
#include <LingmoSession/Component.h>
#include <LingmoSession/CrashRecovery.h>
#include <QProcess>
#include <QTimer>
#include <QDebug>
namespace Lingmo {
static SessionManager *s_instance = nullptr;
SessionManager::SessionManager(QObject *parent)
    : QObject(parent), d(std::make_unique<SessionManagerPrivate>())
{
    d->crashRecovery = std::make_unique<CrashRecovery>(this);
    d->restartTimer = new QTimer(this);
    d->restartTimer->setSingleShot(true);
    connect(d->restartTimer, &QTimer::timeout, this, [this]() {
        for (auto it = d->components.constBegin(); it != d->components.constEnd(); ++it) {
            if (it.value()->state() == ComponentState::Failed)
                restartComponent(it.key());
        }
    });
}
SessionManager::~SessionManager() = default;
SessionManager *SessionManager::instance() {
    if (!s_instance) s_instance = new SessionManager;
    return s_instance;
}
Lingmo::SessionState SessionManager::state() const { return d->state; }
void SessionManager::registerComponent(Component *component) {
    if (!component) return;
    d->components.insert(component->name(), component);
    connect(component, &Component::stateChanged, this, [this, name = component->name()](ComponentState state) {
        if (state == ComponentState::Ready || state == ComponentState::Running) {
            if (!d->readyComponents.contains(name)) {
                d->readyComponents.append(name);
                emit componentStarted(name);
                emit componentsRunningChanged();
                tryStartPending();
            }
        } else if (state == ComponentState::Failed) {
            emit componentFailed(name, QStringLiteral("Component failed"));
        }
    });
}
bool SessionManager::startSession() {
    if (d->state == SessionState::Running || d->state == SessionState::Starting)
        return false;
    d->state = SessionState::Starting;
    emit stateChanged(d->state);
    d->readyComponents.clear();
    tryStartPending();
    return true;
}
void SessionManager::tryStartPending() {
    bool allReady = true;
    for (auto it = d->components.constBegin(); it != d->components.constEnd(); ++it) {
        Component *comp = it.value();
        if (comp->state() == ComponentState::Stopped || comp->state() == ComponentState::Failed) {
            if (comp->dependenciesSatisfied(d->readyComponents)) {
                qInfo("LingmoSession: Starting component: %s", qPrintable(comp->name()));
                comp->setState(ComponentState::Starting);
                QProcess::startDetached(comp->command());
                comp->setState(ComponentState::Running);
            } else {
                allReady = false;
            }
        }
    }
    if (allReady && d->state == SessionState::Starting) {
        d->state = SessionState::Running;
        emit stateChanged(d->state);
        emit sessionReady();
    }
}
void SessionManager::stopSession() {
    if (d->state == SessionState::Stopping)
        return;
    d->state = SessionState::Stopping;
    emit stateChanged(d->state);
    QStringList names = d->components.keys();
    for (int i = names.size() - 1; i >= 0; --i) {
        Component *comp = d->components.value(names.at(i));
        if (comp->state() == ComponentState::Running) {
            comp->setState(ComponentState::Stopped);
            emit componentStopped(comp->name());
        }
    }
    d->state = SessionState::Idle;
    emit stateChanged(d->state);
    emit sessionStopped();
}
bool SessionManager::restartComponent(const QString &name) {
    auto it = d->components.find(name);
    if (it == d->components.end()) return false;
    Component *comp = it.value();
    if (d->crashRecovery->isCrashLimitExceeded(name)) {
        qWarning("LingmoSession: Crash limit exceeded for %s", qPrintable(name));
        emit componentFailed(name, QStringLiteral("Crash limit exceeded"));
        return false;
    }
    d->crashRecovery->recordCrash(name);
    comp->setState(ComponentState::Stopped);
    qInfo("LingmoSession: Restarting component: %s", qPrintable(name));
    comp->setState(ComponentState::Starting);
    QProcess::startDetached(comp->command());
    comp->setState(ComponentState::Running);
    return true;
}
Component *SessionManager::component(const QString &name) const {
    return d->components.value(name);
}
QStringList SessionManager::componentNames() const { return d->components.keys(); }
int SessionManager::componentsRunning() const {
    int count = 0;
    for (auto it = d->components.constBegin(); it != d->components.constEnd(); ++it) {
        if (it.value()->state() == ComponentState::Running)
            ++count;
    }
    return count;
}
void SessionManager::setMaxRestarts(int max) { d->maxRestarts = max; }
void SessionManager::setRestartDelay(int ms) { d->restartDelayMs = ms; }
} // namespace Lingmo
