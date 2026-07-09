#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <LingmoSession/Types.h>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

namespace Lingmo {

class ComponentPrivate;

// A desktop component managed by the session manager.
// Each component has a name, command, dependencies, and state.
class LINGMOSESSION_EXPORT Component : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString command READ command CONSTANT)
    Q_PROPERTY(Lingmo::ComponentState state READ state NOTIFY stateChanged)

public:
    explicit Component(const QString &name, const QString &command,
                       const QStringList &dependencies = {},
                       QObject *parent = nullptr);
    ~Component() override;

    QString name() const;
    QString command() const;
    QStringList dependencies() const;

    Lingmo::ComponentState state() const;

    // Check if all dependencies are satisfied
    bool dependenciesSatisfied(const QStringList &readyComponents) const;

Q_SIGNALS:
    void stateChanged(Lingmo::ComponentState state);
    void started();
    void stopped();
    void failed(const QString &error);

private:
    std::unique_ptr<ComponentPrivate> d;
};

} // namespace Lingmo
