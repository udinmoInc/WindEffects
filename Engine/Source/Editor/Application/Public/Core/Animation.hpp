#pragma once

#include "Geometry.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <cmath>

namespace we::UI {

// Easing functions for smooth animations
namespace Easing {
    inline float Linear(float t) { return t; }
    inline float EaseInQuad(float t) { return t * t; }
    inline float EaseOutQuad(float t) { return t * (2.0f - t); }
    inline float EaseInOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }
    inline float EaseInCubic(float t) { return t * t * t; }
    inline float EaseOutCubic(float t) { return (--t) * t * t + 1.0f; }
    inline float EaseInOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f; }
    inline float EaseOutElastic(float t) {
        const float c4 = (2.0f * 3.14159f) / 3.0f;
        return t == 0.0f ? 0.0f : t == 1.0f ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
    }
}

// Animation base class
class Animation {
public:
    using OnUpdateCallback = std::function<void(float)>;
    using OnCompleteCallback = std::function<void()>;
    
    Animation(float duration, bool loop = false)
        : m_Duration(duration)
        , m_Loop(loop)
        , m_Easing(Easing::EaseOutQuad)
    {}
    
    virtual ~Animation() = default;
    
    void Update(float deltaTime) {
        if (!m_Playing || m_Duration <= 0.0f) return;
        
        m_ElapsedTime += deltaTime;
        
        if (m_ElapsedTime >= m_Duration) {
            if (m_Loop) {
                m_ElapsedTime = 0.0f;
            } else {
                m_ElapsedTime = m_Duration;
                m_Playing = false;
                if (m_OnComplete) m_OnComplete();
            }
        }
        
        float t = m_ElapsedTime / m_Duration;
        float easedT = m_Easing(t);
        
        if (m_OnUpdate) m_OnUpdate(easedT);
    }
    
    void Play() { m_Playing = true; }
    void Pause() { m_Playing = false; }
    void Stop() { m_Playing = false; m_ElapsedTime = 0.0f; }
    void Reset() { m_ElapsedTime = 0.0f; }
    
    bool IsPlaying() const { return m_Playing; }
    bool IsComplete() const { return !m_Playing && m_ElapsedTime >= m_Duration; }
    float GetProgress() const { return m_Duration > 0.0f ? m_ElapsedTime / m_Duration : 0.0f; }
    
    void SetEasing(std::function<float(float)> easing) { m_Easing = easing; }
    void SetOnUpdate(OnUpdateCallback callback) { m_OnUpdate = callback; }
    void SetOnComplete(OnCompleteCallback callback) { m_OnComplete = callback; }
    
protected:
    float m_Duration;
    float m_ElapsedTime = 0.0f;
    bool m_Playing = false;
    bool m_Loop = false;
    std::function<float(float)> m_Easing;
    OnUpdateCallback m_OnUpdate;
    OnCompleteCallback m_OnComplete;
};

// Float animation (for colors, opacity, etc.)
class FloatAnimation : public Animation {
public:
    FloatAnimation(float from, float to, float duration, bool loop = false)
        : Animation(duration, loop)
        , m_From(from)
        , m_To(to)
    {}
    
    float GetValue() const {
        float t = GetProgress();
        float easedT = m_Easing(t);
        return m_From + (m_To - m_From) * easedT;
    }
    
protected:
    float m_From;
    float m_To;
};

// Color animation
struct Color;
class ColorAnimation : public Animation {
public:
    ColorAnimation(Color from, Color to, float duration, bool loop = false);
    
    Color GetValue() const;
    
protected:
    Color m_From;
    Color m_To;
};

// Animation manager
class AnimationManager {
public:
    static AnimationManager& Get();
    
    void Update(float deltaTime) {
        for (auto& anim : m_Animations) {
            anim->Update(deltaTime);
        }
        
        // Remove completed animations
        m_Animations.erase(
            std::remove_if(m_Animations.begin(), m_Animations.end(),
                [](const std::shared_ptr<Animation>& anim) {
                    return anim->IsComplete();
                }),
            m_Animations.end()
        );
    }
    
    void AddAnimation(const std::shared_ptr<Animation>& animation) {
        m_Animations.push_back(animation);
        animation->Play();
    }
    
    void Clear() { m_Animations.clear(); }
    
private:
    AnimationManager() = default;
    std::vector<std::shared_ptr<Animation>> m_Animations;
};

// Convenience functions for common animations
inline void AnimateFloat(float* value, float to, float duration = 0.15f) {
    auto anim = std::make_shared<FloatAnimation>(*value, to, duration);
    anim->SetOnUpdate([value](float t) {
        // Will be handled by GetValue in a more complete implementation
    });
    AnimationManager::Get().AddAnimation(anim);
}

} // namespace we::editor::application::UI
