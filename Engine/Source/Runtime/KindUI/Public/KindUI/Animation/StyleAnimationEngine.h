#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/InteractionState.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Styles/WidgetStyles.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Tokens/DesignToken.h"

#include <cmath>
#include <unordered_map>

namespace we::runtime::kindui {

// Interpolates resolved visual styles. Themes define timing; KindUI performs interpolation.
class KINDUI_API StyleAnimationEngine {
public:
    struct AnimatedStyle {
        ResolvedVisualStyle current{};
        ResolvedVisualStyle target{};
        float progress = 1.0f;
        float duration = 0.15f;
        AnimationToken timingToken = AnimationToken::Fast;
    };

    void SetTheme(const IKindUITheme* theme) { m_Theme = theme; }

    // Begin or update a transition toward the target style for a widget instance.
    void TransitionTo(uint64_t widgetId, const ResolvedVisualStyle& target, AnimationToken timing);

    // Tick all active transitions. Returns true if any animation is still running.
    bool Tick(float deltaTime);

    // Get the current interpolated style for a widget (returns target if no animation active).
    [[nodiscard]] ResolvedVisualStyle GetCurrent(uint64_t widgetId) const;

    void Clear(uint64_t widgetId);
    void ClearAll();

private:
    const IKindUITheme* m_Theme = nullptr;
    std::unordered_map<uint64_t, AnimatedStyle> m_Animations{};

    [[nodiscard]] static ResolvedVisualStyle Lerp(
        const ResolvedVisualStyle& from,
        const ResolvedVisualStyle& to,
        float t);
};

} // namespace we::runtime::kindui
