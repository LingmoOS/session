// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <LingmoSession/ShellLauncher.h>

#include <QProcess>
#include <QString>

namespace Lingmo {

class ShellLauncherPrivate
{
public:
    QProcess *process = nullptr;
    QString   program;
};

} // namespace Lingmo
