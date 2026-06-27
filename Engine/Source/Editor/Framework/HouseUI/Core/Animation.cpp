#include "Animation.hpp"
#include "Geometry.hpp"

namespace HouseEngine::UI {

AnimationManager& AnimationManager::Get() {
    static AnimationManager instance;
    return instance;
}

ColorAnimation::ColorAnimation(Color from, Color to, float duration, bool loop)
    : Animation(duration, loop)
    , m_From(from)
    , m_To(to)
{}

Color ColorAnimation::GetValue() const {
    float t = GetProgress();
    float easedT = m_Easing(t);
    
    return Color{
        m_From.r + (m_To.r - m_From.r) * easedT,
        m_From.g + (m_To.g - m_From.g) * easedT,
        m_From.b + (m_To.b - m_From.b) * easedT,
        m_From.a + (m_To.a - m_From.a) * easedT
    };
}

} // namespace HouseEngine::UI
