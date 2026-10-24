#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <QObject>
#include <QString>

namespace Lingmo {

class RestartGuardPrivate;

// Enforces a bounded restart policy for a session process.
//
// A process that crashes quickly counts against its budget; one that stays
// up for at least `stableUptimeMs` resets its counter. Once the budget is
// exhausted the guard emits gaveUp() and refuses further restarts.
//
// Usage:
//   RestartGuard guard("lingmo-services");
//   connect(&guard, &RestartGuard::shouldRestart, this, &Self::startServices);
//   connect(&guard, &RestartGuard::gaveUp, this, &Self::onServicesGaveUp);
//   guard.notifyStarted();
//   // on process exit:
//   guard.notifyExited(code, crashed);
class LINGMOSESSION_EXPORT RestartGuard : public QObject
{
    Q_OBJECT
public:
    struct Policy {
        int maxRestarts = 5;
        int stableUptimeMs = 30'000; // reset counter after this much uptime
        int initialDelayMs = 1'000;
        int backoffMs      = 2'000;  // added per retry
    };

    explicit RestartGuard(const QString &name,
                          const Policy &policy = {},
                          QObject *parent = nullptr);
    ~RestartGuard() override;

    // Call on every start, including the first.
    void notifyStarted();

    // Call when the monitored process exits.
    void notifyExited(int exitCode, bool crashed);

    // Permanently cancel pending and future restarts.
    void cancel();

    bool isPending() const;
    bool hasGivenUp() const;

Q_SIGNALS:
    void shouldRestart();
    void gaveUp();

private:
    std::unique_ptr<RestartGuardPrivate> d;
};

} // namespace Lingmo
