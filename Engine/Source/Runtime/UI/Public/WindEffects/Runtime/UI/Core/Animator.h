#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/UIRepaintGate.h"

#include <cmath>
#include <algorithm>

namespace WindEffects::Editor::UI {

class UI_API Animator {
public:
    static void Tick(float deltaTime) {
        s_DeltaTime = deltaTime;
    }

    // Smooth dampening towards a target value (time-independent easing).
    // Snaps when settled so idle frames can skip UI rebuilds.
    static float Damp(float current, float target, float speed, float dt) {
        const float delta = target - current;
        if (std::abs(delta) < 0.001f) {
            if (current != target) {
                UIRepaintGate::Request();
            }
            return target;
        }
        UIRepaintGate::MarkAnimating();
        return std::lerp(current, target, 1.0f - std::exp(-speed * dt));
    }

    static float Damp(float current, float target, float speed = 15.0f) {
        return Damp(current, target, speed, s_DeltaTime);
    }

    static float MoveTowards(float current, float target, float maxDelta) {
        if (std::abs(target - current) <= maxDelta) {
            return target;
        }
        UIRepaintGate::MarkAnimating();
        return current + std::copysign(maxDelta, target - current);
    }

private:
    static inline float s_DeltaTime = 0.016f;
};

} // namespace WindEffects::Editor::UI
