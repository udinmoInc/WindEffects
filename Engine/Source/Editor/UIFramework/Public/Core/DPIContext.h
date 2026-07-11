#pragma once

#include "WindEffects/Editor/UI/Export.h"

namespace WindEffects::Editor::UI {

class DPIContext {
public:
    UIFRAMEWORK_API static float GetScale();
    UIFRAMEWORK_API static void SetScale(float scale);

    static float Scale(float value) { return value * GetScale(); }
};

} // namespace WindEffects::Editor::UI
