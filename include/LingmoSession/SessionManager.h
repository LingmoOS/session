#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <LingmoSession/Types.h>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <memory>

namespace Lingmo {

class Component;
class SessionManagerPrivate;

// Central session manager that orchestrates the desktop lifecycle.
//
// Responsibilities:
// - Start components in dependency order
// - Monitor component health
// - Handle crashes with automatic restart
// - Manage environment setup
// - Coordinate shutdown
class LINGMOSESSION_EXPORT SessionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Lingmo::SessionState state READ state NOTIFY stateChanged)
    Q_PROPERTY(int componentsRunning READ componentsRunning NOTIFY componentsRunningChanged)

public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager() override;

    static SessionManager *instance();

    Lingmo::SessionState state() const;

    // Register a component to be managed
    void registerComponent(Component *component);

    // Start the session (launches all registered components in order)
    bool startSession();

    // Stop the session gracefully
    void stopSession();

    // Restart a crashed component
    bool restartComponent(const QString &name);

    // Get a registered component by name
    Component *component(const QString &name) const;

    // Get all component names
    QStringList componentNames() const;

    int componentsRunning() const;

    // Set maximum restart attempts before giving up
    void setMaxRestarts(int max);

    // Set restart delay in milliseconds
    void setRestartDelay(int ms);

Q_SIGNALS:
    void stateChanged(Lingmo::SessionState state);
    void componentsRunningChanged();
    void componentStarted(const QString &name);
    void componentStopped(const QString &name);
    void componentFailed(const QString &name, const QString &error);
    void sessionReady();
    void sessionStopped();

private:
    void tryStartPending();
    std::unique_ptr<SessionManagerPrivate> d;
};

} // namespace Lingmo
