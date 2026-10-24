// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <LingmoSession/Component.h>
#include <LingmoSession/CrashRecovery.h>
#include <LingmoSession/RestartGuard.h>
#include <LingmoSession/SessionManager.h>

#include <QMap>
#include <QProcess>
#include <QTimer>

namespace Lingmo {

class SessionManagerPrivate
{
public:
    Lingmo::SessionState              state          = Lingmo::SessionState::Idle;
    QMap<QString, Component *>        components;
    QMap<QString, QProcess *>         processes;
    QMap<QString, RestartGuard *>     guards;
    std::unique_ptr<CrashRecovery>    crashRecovery;
    int                               maxRestarts    = 3;
    int                               restartDelayMs = 2000;
    QTimer                           *restartTimer   = nullptr;
    QStringList                       readyComponents;
};

} // namespace Lingmo
