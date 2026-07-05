#pragma once

#include "Application/Export.hpp"

namespace we::UI {

class DPIContext {
public:
    APPLICATION_API static float GetScale();
    APPLICATION_API static void SetScale(float scale);

    static float Scale(float value) { return value * GetScale(); }
};

} // namespace we::UI
