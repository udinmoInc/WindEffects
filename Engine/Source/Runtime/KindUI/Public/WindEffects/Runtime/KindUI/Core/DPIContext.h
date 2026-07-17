#pragma once

#include "WindEffects/Runtime/UI/Export.h"

namespace WindEffects::Editor::UI {

class DPIContext {
public:
    UI_API static float GetScale();
    UI_API static void SetScale(float scale);

    static float Scale(float value) { return value * GetScale(); }
};

} // namespace WindEffects::Editor::UI
