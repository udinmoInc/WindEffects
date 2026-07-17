#include "KindUI/Core/DPIContext.h"

namespace we::runtime::kindui {

namespace {

float s_DPIScale = 1.0f;

} // namespace

float DPIContext::GetScale() {
    return s_DPIScale;
}

void DPIContext::SetScale(float scale) {
    s_DPIScale = scale;
}

} // namespace we::runtime::kindui
