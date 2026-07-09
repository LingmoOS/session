#include "private/CrashRecovery_p.h"
namespace Lingmo {
CrashRecovery::CrashRecovery(QObject *parent)
    : QObject(parent), d(std::make_unique<CrashRecoveryPrivate>()) {}
CrashRecovery::~CrashRecovery() = default;
void CrashRecovery::recordCrash(const QString &name) {
    int &count = d->crashCounts[name];
    ++count;
    if (count >= d->maxCrashes)
        emit crashLimitExceeded(name, count);
}
bool CrashRecovery::isCrashLimitExceeded(const QString &name) const {
    return d->crashCounts.value(name, 0) >= d->maxCrashes;
}
int CrashRecovery::crashCount(const QString &name) const {
    return d->crashCounts.value(name, 0);
}
void CrashRecovery::resetCrashCount(const QString &name) {
    d->crashCounts.remove(name);
}
void CrashRecovery::resetAll() { d->crashCounts.clear(); }
void CrashRecovery::setMaxCrashes(int max) { d->maxCrashes = max; }
} // namespace Lingmo
