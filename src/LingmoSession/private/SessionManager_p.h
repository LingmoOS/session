#pragma once
#include <LingmoSession/SessionManager.h>
#include <LingmoSession/Component.h>
#include <LingmoSession/CrashRecovery.h>
#include <QMap>
#include <QTimer>
#include <memory>
namespace Lingmo {
class SessionManagerPrivate {
public:
    Lingmo::SessionState state = Lingmo::SessionState::Idle;
    QMap<QString, Component*> components;
    std::unique_ptr<CrashRecovery> crashRecovery;
    int maxRestarts = 3;
    int restartDelayMs = 2000;
    QTimer *restartTimer = nullptr;
    QStringList readyComponents;
};
} // namespace Lingmo
