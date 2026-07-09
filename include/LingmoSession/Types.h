#pragma once
#include <LingmoSession/LingmoSessionExport.h>
#include <QObject>

namespace Lingmo {
Q_NAMESPACE

enum class SessionState {
    Idle,
    Starting,
    Running,
    Stopping,
    Crashed,
    Failed
};
Q_ENUM_NS(SessionState)

enum class ComponentState {
    Stopped,
    Starting,
    Ready,
    Running,
    Failed
};
Q_ENUM_NS(ComponentState)

} // namespace Lingmo
