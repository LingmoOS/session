#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <QObject>

namespace Lingmo {

// Sets up the session environment variables and directories.
//
// Responsibilities:
// - Create runtime directories
// - Set LINGMO_* environment variables
// - Ensure XDG directories exist
class LINGMOSESSION_EXPORT EnvironmentSetup : public QObject
{
    Q_OBJECT

public:
    explicit EnvironmentSetup(QObject *parent = nullptr);
    ~EnvironmentSetup() override;

    // Set up the complete session environment
    bool setup();

    // Create runtime directories
    bool createRuntimeDirs();

    // Set Lingmo environment variables
    void setEnvironmentVariables();

private:
    bool ensureXdgDirs();
};

} // namespace Lingmo
