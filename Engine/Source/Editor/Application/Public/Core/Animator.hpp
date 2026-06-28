#pragma once

#include <cmath>
#include <algorithm>

namespace we::UI {

class Animator {
public:
    static void Tick(float deltaTime) {
        s_DeltaTime = deltaTime;
    }
    
    // Smooth dampening towards a target value (time-independent easing)
    static float Damp(float current, float target, float speed, float dt) {
        return std::lerp(current, target, 1.0f - std::exp(-speed * dt));
    }
    
    // Quick helper using global delta time
    static float Damp(float current, float target, float speed = 15.0f) {
        return Damp(current, target, speed, s_DeltaTime);
    }
    
    // Linear transition
    static float MoveTowards(float current, float target, float maxDelta) {
        if (std::abs(target - current) <= maxDelta) return target;
        return current + std::copysign(maxDelta, target - current);
    }

private:
    static inline float s_DeltaTime = 0.016f;
};

} // namespace we::editor::application::UI
