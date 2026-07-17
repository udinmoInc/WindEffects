#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/InteractionState.h"
#include "KindUI/Core/WidgetVariant.h"
#include "KindUI/Styles/WidgetStyles.h"
#include "KindUI/Theming/IKindUITheme.h"

#include <optional>
#include <string>
#include <string_view>

namespace we::runtime::kindui {

// Context passed through the style resolution pipeline.
struct StyleContext {
    const IKindUITheme* theme = nullptr;
    float dpiScale = 1.0f;

    // Layer inputs (each may be null / empty to skip that layer).
    const ButtonStyle* frameworkDefault = nullptr;
    const ButtonStyle* applicationDefault = nullptr;
    std::string_view styleClass{};
    WidgetVariant variant = WidgetVariant::Default;
    InteractionStateSet interaction{};
    const ButtonStyle* localOverride = nullptr;
};

class KINDUI_API StylePipeline {
public:
    // Resolve a ButtonStyle through all layers, returning the merged style definition.
    [[nodiscard]] static ButtonStyle ResolveButtonStyle(const StyleContext& ctx);

    // Resolve a ButtonStyle definition into concrete visual properties for rendering.
    [[nodiscard]] static ResolvedVisualStyle ResolveButtonVisual(
        const ButtonStyle& style,
        const StyleContext& ctx);

    // Merge two style definitions: `over` wins over `base` for each property.
    [[nodiscard]] static ButtonStyle MergeButtonStyles(const ButtonStyle& base, const ButtonStyle& over);

    // Apply interaction state to a resolved visual style.
    [[nodiscard]] static ResolvedVisualStyle ApplyInteractionState(
        const ResolvedVisualStyle& base,
        const ButtonStyle& styleDef,
        const StyleContext& ctx);
};

} // namespace we::runtime::kindui
