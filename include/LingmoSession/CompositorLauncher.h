#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <QObject>
#include <QString>

namespace Lingmo {

class CompositorLauncherPrivate;

// Launches the Wayland compositor and waits for it to signal readiness
// through an inherited pipe. The compositor writes a single byte ('R')
// once its display is up; a startup timeout fires if it never does.
//
// Unlike a simple QProcess wrapper, this owns the readiness-pipe lifecycle
// and translates compositor exit into one of three outcomes: ready, failed,
// or exited-after-ready.
class LINGMOSESSION_EXPORT CompositorLauncher : public QObject
{
    Q_OBJECT
public:
    explicit CompositorLauncher(QObject *parent = nullptr);
    ~CompositorLauncher() override;

    // Launch the compositor binary. `socketName` is the WAYLAND_DISPLAY to
    // use. `controlSocket` is the compositor's shell-control socket path
    // (may be empty). `xwaylandReadyFile` is a file the compositor touches
    // once Xwayland is available (may be empty).
    void launch(const QString &binary, const QString &socketName,
                const QString &controlSocket = {},
                int timeoutMs = 15000,
                const QString &xwaylandReadyFile = {});

    // Graceful shutdown: SIGTERM, then SIGKILL after 3 s.
    void stop();

    bool isReady() const;
    bool isRunning() const;

Q_SIGNALS:
    void ready();
    void failed(const QString &reason);
    void exited(int exitCode);

private:
    std::unique_ptr<CompositorLauncherPrivate> d;
};

} // namespace Lingmo
