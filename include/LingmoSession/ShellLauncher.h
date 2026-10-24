#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <QObject>
#include <QString>
#include <QStringList>

namespace Lingmo {

class ShellLauncherPrivate;

// Launches the shell process (file manager daemon) on a privileged Wayland
// connection obtained through ShellChannel. The shell draws the desktop and
// hosts panel/applet UI. It can be restarted without restarting the compositor
// because the control channel persists across shell generations.
class LINGMOSESSION_EXPORT ShellLauncher : public QObject
{
    Q_OBJECT
public:
    explicit ShellLauncher(QObject *parent = nullptr);
    ~ShellLauncher() override;

    // Launch `program --daemon`. If `channel` is connected, a privileged
    // Wayland socketpair is created and passed to the daemon; otherwise the
    // daemon runs without a desktop.
    bool launch(const QString &program, const QString &waylandSocket,
                ShellChannel *channel = nullptr);

    void stop();

    bool isRunning() const;

Q_SIGNALS:
    void finished(int exitCode, bool crashed);

private:
    std::unique_ptr<ShellLauncherPrivate> d;
};

} // namespace Lingmo
