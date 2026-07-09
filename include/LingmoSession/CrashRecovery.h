#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <QObject>
#include <QString>
#include <memory>

namespace Lingmo {

class CrashRecoveryPrivate;

// Handles crash detection and recovery for session components.
//
// Tracks component crash history and implements restart policies.
class LINGMOSESSION_EXPORT CrashRecovery : public QObject
{
    Q_OBJECT

public:
    explicit CrashRecovery(QObject *parent = nullptr);
    ~CrashRecovery() override;

    // Record a crash for a component
    void recordCrash(const QString &componentName);

    // Check if a component has exceeded its crash limit
    bool isCrashLimitExceeded(const QString &componentName) const;

    // Get crash count for a component
    int crashCount(const QString &componentName) const;

    // Reset crash count for a component
    void resetCrashCount(const QString &componentName);

    // Reset all crash counts
    void resetAll();

    void setMaxCrashes(int max);

Q_SIGNALS:
    void crashLimitExceeded(const QString &componentName, int crashCount);

private:
    std::unique_ptr<CrashRecoveryPrivate> d;
};

} // namespace Lingmo
