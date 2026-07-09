#include "private/Component_p.h"
namespace Lingmo {
Component::Component(const QString &name, const QString &command,
                     const QStringList &dependencies, QObject *parent)
    : QObject(parent), d(std::make_unique<ComponentPrivate>())
{
    d->name = name;
    d->command = command;
    d->dependencies = dependencies;
}
Component::~Component() = default;
QString Component::name() const { return d->name; }
QString Component::command() const { return d->command; }
QStringList Component::dependencies() const { return d->dependencies; }
Lingmo::ComponentState Component::state() const { return d->state; }
void Component::setState(Lingmo::ComponentState state) {
    if (d->state != state) {
        d->state = state;
        emit stateChanged(state);
    }
}
bool Component::dependenciesSatisfied(const QStringList &readyComponents) const
{
    for (const QString &dep : d->dependencies) {
        if (!readyComponents.contains(dep))
            return false;
    }
    return true;
}
} // namespace Lingmo
