#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

namespace Lingmo {

class AutostartManagerPrivate;

// Manages XDG autostart applications.
//
// Scans autostart directories and launches applications
// that should start automatically when the session begins.
class LINGMOSESSION_EXPORT AutostartManager : public QObject
{
    Q_OBJECT

public:
    explicit AutostartManager(QObject *parent = nullptr);
    ~AutostartManager() override;

    // Discover autostart applications
    void discover();

    // Launch all discovered autostart applications
    void launchAll();

    // Get list of autostart application names
    QStringList applications() const;

Q_SIGNALS:
    void applicationLaunched(const QString &name);
    void applicationFailed(const QString &name, const QString &error);

private:
    std::unique_ptr<AutostartManagerPrivate> d;
};

} // namespace Lingmo
