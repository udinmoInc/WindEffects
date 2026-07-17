#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

// Semantic widget variants. Applications choose variants; themes decide appearance.
enum class WidgetVariant : uint32_t {
    Default,
    Primary,
    Secondary,
    Toolbar,
    Flat,
    Ghost,
    Link,
    Outline,
    Success,
    Warning,
    Danger,
    Accent,
};

} // namespace we::runtime::kindui
