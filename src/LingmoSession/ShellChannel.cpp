// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "private/ShellChannel_p.h"

#include <LingmoSession/ShellChannel.h>

#include <QDeadlineTimer>
#include <QLoggingCategory>
#include <QThread>

#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(lcShellChannel, "lingmo.session.shellchannel")

namespace Lingmo {

ShellChannel::ShellChannel(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ShellChannelPrivate>())
{
}

ShellChannel::~ShellChannel()
{
    if (d->fd >= 0)
        ::close(d->fd);
}

bool ShellChannel::connectToCompositor(const QString &path, int timeoutMs)
{
    const QByteArray pathBytes = path.toUtf8();

    sockaddr_un address {};
    address.sun_family = AF_UNIX;
    if (static_cast<size_t>(pathBytes.size()) >= sizeof(address.sun_path))
    {
        qCWarning(lcShellChannel) << "Compositor control socket path too long:" << path;
        return false;
    }
    std::memcpy(address.sun_path, pathBytes.constData(), pathBytes.size());

    QDeadlineTimer deadline(timeoutMs);
    while (true)
    {
        const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0)
            return false;

        if (::connect(fd,
                      reinterpret_cast<sockaddr *>(&address),
                      sizeof(address)) == 0)
        {
            d->fd = fd;
            return true;
        }
        ::close(fd);

        if (deadline.hasExpired())
        {
            qCWarning(lcShellChannel) << "Cannot reach compositor control socket:" << path;
            return false;
        }
        QThread::msleep(50);
    }
}

bool ShellChannel::isConnected() const
{
    return d->fd >= 0;
}

int ShellChannel::createShellConnection()
{
    if (d->fd < 0)
        return -1;

    int pair[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, pair) != 0)
    {
        qCWarning(lcShellChannel) << "Cannot create shell Wayland socketpair";
        return -1;
    }

    // Send pair[0] to the compositor via SCM_RIGHTS.
    char payload = 'S';
    iovec iov {};
    iov.iov_base = &payload;
    iov.iov_len  = 1;

    union
    {
        char    buffer[CMSG_SPACE(sizeof(int))];
        cmsghdr alignment;
    } control {};

    msghdr message {};
    message.msg_iov        = &iov;
    message.msg_iovlen     = 1;
    message.msg_control    = control.buffer;
    message.msg_controllen = sizeof(control.buffer);

    cmsghdr *hdr     = CMSG_FIRSTHDR(&message);
    hdr->cmsg_level  = SOL_SOCKET;
    hdr->cmsg_type   = SCM_RIGHTS;
    hdr->cmsg_len    = CMSG_LEN(sizeof(int));
    std::memcpy(CMSG_DATA(hdr), &pair[0], sizeof(int));

    const bool sent = (::sendmsg(d->fd, &message, MSG_NOSIGNAL) == 1);
    ::close(pair[0]);

    if (!sent)
    {
        qCWarning(lcShellChannel) << "Cannot pass shell connection to compositor";
        ::close(pair[1]);
        return -1;
    }

    return pair[1];
}

} // namespace Lingmo
