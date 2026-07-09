#pragma once
#include <LingmoSession/Component.h>
namespace Lingmo {
class ComponentPrivate {
public:
    QString name;
    QString command;
    QStringList dependencies;
    Lingmo::ComponentState state = Lingmo::ComponentState::Stopped;
    int pid = -1;
};
} // namespace Lingmo
