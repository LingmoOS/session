#pragma once
#include <LingmoSession/CrashRecovery.h>
#include <QMap>
namespace Lingmo {
class CrashRecoveryPrivate {
public:
    QMap<QString, int> crashCounts;
    int maxCrashes = 3;
};
} // namespace Lingmo
