#pragma once

#include "KindUI/Export.h"

namespace we::runtime::kindui {

class DPIContext {
public:
    KINDUI_API static float GetScale();
    KINDUI_API static void SetScale(float scale);

    static float Scale(float value) { return value * GetScale(); }
};

} // namespace we::runtime::kindui
