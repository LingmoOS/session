#pragma once
#include <LingmoSession/AutostartManager.h>
#include <QStringList>
namespace Lingmo {
class AutostartManagerPrivate {
public:
    QStringList searchPaths;
    QStringList discoveredApps;
};
} // namespace Lingmo
