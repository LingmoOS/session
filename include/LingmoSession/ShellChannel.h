#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <QObject>
#include <QString>

namespace Lingmo {

class ShellChannelPrivate;

// The session's end of the compositor control channel.
//
// The session (not the compositor) launches the shell. For every launch it
// creates a Wayland socketpair, sends one end to the compositor over
// SCM_RIGHTS — the compositor turns it into a wl_client and treats that
// client, and only that client, as the trusted shell — and hands the other
// end to the shell process as an inherited file descriptor.
//
// The channel is held open for the life of the session so a shell restart
// needs no compositor restart.
class LINGMOSESSION_EXPORT ShellChannel : public QObject
{
    Q_OBJECT
public:
    explicit ShellChannel(QObject *parent = nullptr);
    ~ShellChannel() override;

    // Connect to the compositor's control socket, retrying for `timeoutMs`
    // while the compositor is still creating it.
    bool connectToCompositor(const QString &path, int timeoutMs = 5000);

    bool isConnected() const;

    // Create a Wayland socketpair, give one end to the compositor and return
    // the other for the shell process. Returns -1 on failure; caller owns the
    // returned descriptor.
    int createShellConnection();

private:
    std::unique_ptr<ShellChannelPrivate> d;
};

} // namespace Lingmo
