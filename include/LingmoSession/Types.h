#pragma once
#include <LingmoSession/LingmoSessionExport.h>

namespace Lingmo {

enum class SessionState {
    Idle,
    Starting,
    Running,
    Stopping,
    Crashed,
    Failed
};

enum class ComponentState {
    Stopped,
    Starting,
    Ready,
    Running,
    Failed
};

} // namespace Lingmo
