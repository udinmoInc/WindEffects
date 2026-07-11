#include "Core/DPIContext.h"

namespace WindEffects::Editor::UI {

namespace {

float s_DPIScale = 1.0f;

} // namespace

float DPIContext::GetScale() {
    return s_DPIScale;
}

void DPIContext::SetScale(float scale) {
    s_DPIScale = scale;
}

} // namespace WindEffects::Editor::UI
