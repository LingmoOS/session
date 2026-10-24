// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <LingmoSession/RestartGuard.h>

#include <QElapsedTimer>
#include <QTimer>

namespace Lingmo {

class RestartGuardPrivate
{
public:
    QString              name;
    RestartGuard::Policy policy;
    QTimer              *retryTimer  = nullptr;
    QElapsedTimer        uptimeTimer;
    int                  restarts    = 0;
    bool                 cancelled   = false;
    bool                 gaveUp      = false;
};

} // namespace Lingmo
