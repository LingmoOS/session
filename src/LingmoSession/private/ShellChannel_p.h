// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <LingmoSession/ShellChannel.h>

namespace Lingmo {

class ShellChannelPrivate
{
public:
    int fd = -1; // connected Unix socket fd
};

} // namespace Lingmo
