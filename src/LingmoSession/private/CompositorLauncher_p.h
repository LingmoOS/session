// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <LingmoSession/CompositorLauncher.h>

#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>

namespace Lingmo {

class CompositorLauncherPrivate
{
public:
    QProcess          *process       = nullptr;
    QSocketNotifier   *readyNotifier = nullptr;
    QTimer            *timeoutTimer  = nullptr;
    int                readFd        = -1;
    bool               ready         = false;
    bool               stopping      = false;
};

} // namespace Lingmo
