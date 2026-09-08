#include "KindUI/Core/Animator.h"

namespace we::runtime::kindui {

static float s_DeltaTime = 0.016f;

void Animator::Tick(float deltaTime) {
    s_DeltaTime = deltaTime;
}

float Animator::GetDeltaTime() {
    return s_DeltaTime;
}

float Animator::Damp(float current, float target, float speed) {
    return Damp(current, target, speed, s_DeltaTime);
}

} // namespace we::runtime::kindui
 
