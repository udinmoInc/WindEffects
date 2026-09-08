#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/UIRepaintGate.h"

#include <cmath>
#include <algorithm>

namespace we::runtime::kindui {

class KINDUI_API Animator {
public:
    static constexpr float kSettleEpsilon = 0.001f;

    static void Tick(float deltaTime);

    /// Smooth dampening towards a target value (time-independent easing).
    /// Snaps when within epsilon so asymptotic tails do not repaint forever.
    static float Damp(float current, float target, float speed, float dt) {
        const float delta = target - current;
        if (std::abs(delta) < kSettleEpsilon) {
            if (current != target) {
                UIRepaintGate::RequestPaint();
            }
            return target;
        }
        const float next = std::lerp(current, target, 1.0f - std::exp(-speed * dt));
        if (std::abs(next - target) < kSettleEpsilon) {
            UIRepaintGate::RequestPaint();
            return target;
        }
        if (std::abs(next - current) >= kSettleEpsilon * 0.25f) {
            UIRepaintGate::RequestPaint();
        }
        return next;
    }

    static float Damp(float current, float target, float speed = 30.0f);

    static float MoveTowards(float current, float target, float maxDelta) {
        if (std::abs(target - current) <= maxDelta) {
            if (current != target) {
                UIRepaintGate::RequestPaint();
            }
            return target;
        }
        const float next = current + std::copysign(maxDelta, target - current);
        if (next != current) {
            UIRepaintGate::RequestPaint();
        }
        return next;
    }

    static float GetDeltaTime();
};

} // namespace we::runtime::kindui
