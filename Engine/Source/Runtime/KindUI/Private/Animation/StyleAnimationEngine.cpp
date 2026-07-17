#include "KindUI/Animation/StyleAnimationEngine.h"

namespace we::runtime::kindui {

ResolvedVisualStyle StyleAnimationEngine::Lerp(
    const ResolvedVisualStyle& from,
    const ResolvedVisualStyle& to,
    float t)
{
    ResolvedVisualStyle result{};
    result.background = Color::Lerp(from.background, to.background, t);
    result.foreground = Color::Lerp(from.foreground, to.foreground, t);
    result.border = Color::Lerp(from.border, to.border, t);
    result.icon = Color::Lerp(from.icon, to.icon, t);
    result.cornerRadius = from.cornerRadius + (to.cornerRadius - from.cornerRadius) * t;
    result.borderWidth = from.borderWidth + (to.borderWidth - from.borderWidth) * t;
    result.fontSize = from.fontSize + (to.fontSize - from.fontSize) * t;
    result.height = from.height + (to.height - from.height) * t;
    result.iconSize = from.iconSize + (to.iconSize - from.iconSize) * t;
    result.elevation = static_cast<int>(static_cast<float>(from.elevation) +
        (static_cast<float>(to.elevation - from.elevation) * t));
    result.opacity = from.opacity + (to.opacity - from.opacity) * t;
    result.scale = from.scale + (to.scale - from.scale) * t;
    result.translation = {
        from.translation.x + (to.translation.x - from.translation.x) * t,
        from.translation.y + (to.translation.y - from.translation.y) * t,
    };
    result.rotation = from.rotation + (to.rotation - from.rotation) * t;
    return result;
}

void StyleAnimationEngine::TransitionTo(
    uint64_t widgetId,
    const ResolvedVisualStyle& target,
    AnimationToken timing)
{
    auto& anim = m_Animations[widgetId];
    if (anim.progress >= 1.0f) {
        anim.current = target;
    } else {
        anim.current = Lerp(anim.current, anim.target, anim.progress);
    }
    anim.target = target;
    anim.progress = 0.0f;
    anim.timingToken = timing;
    anim.duration = m_Theme ? m_Theme->ResolveAnimationDuration(timing) : 0.15f;
}

bool StyleAnimationEngine::Tick(float deltaTime) {
    bool anyActive = false;
    for (auto& [id, anim] : m_Animations) {
        (void)id;
        if (anim.progress >= 1.0f) {
            continue;
        }
        anyActive = true;
        if (anim.duration <= 0.0f) {
            anim.progress = 1.0f;
            anim.current = anim.target;
            continue;
        }
        anim.progress = std::min(1.0f, anim.progress + deltaTime / anim.duration);
        anim.current = Lerp(anim.current, anim.target, anim.progress);
    }
    return anyActive;
}

ResolvedVisualStyle StyleAnimationEngine::GetCurrent(uint64_t widgetId) const {
    const auto it = m_Animations.find(widgetId);
    if (it == m_Animations.end()) {
        return {};
    }
    return it->second.current;
}

void StyleAnimationEngine::Clear(uint64_t widgetId) {
    m_Animations.erase(widgetId);
}

void StyleAnimationEngine::ClearAll() {
    m_Animations.clear();
}

} // namespace we::runtime::kindui
