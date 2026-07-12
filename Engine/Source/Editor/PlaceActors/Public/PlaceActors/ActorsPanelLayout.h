#pragma once

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Rendering/IconMetrics.h"

namespace we::programs::editor::ActorsPanelLayout {

inline float ContentPadH() {
    return WindEffects::Editor::UI::PanelChrome::PanelPaddingH();
}

inline float ContentPadV() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space2);
}

inline float ChevronSize() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::IconSizeTree);
}

inline float ActorRowHeight() {
    return 32.0f * WindEffects::Editor::UI::PanelChrome::UiScale();
}

inline float CategoryHeight() {
    return 28.0f * WindEffects::Editor::UI::PanelChrome::UiScale();
}

inline float IconSize() {
    return static_cast<float>(WindEffects::Editor::UI::IconMetrics::NativeIconTierPx(
        WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::IconSizeTree)));
}

inline float RowRadius() {
    return 5.0f * WindEffects::Editor::UI::PanelChrome::UiScale();
}

inline float SearchHeight() {
    return 28.0f * WindEffects::Editor::UI::PanelChrome::UiScale();
}

inline float SearchRowHeight() {
    return SearchHeight() + ContentPadV() * 2.0f + 4.0f;
}

inline float ItemIndent() {
    return ChevronSize() + WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space2);
}

inline float ItemIconX(float contentX) {
    return contentX + ContentPadH() + ItemIndent();
}

inline float LabelX(float contentX) {
    return ItemIconX(contentX) + IconSize()
        + WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space2);
}

inline float StarIconX(float contentX, float contentWidth) {
    return contentX + contentWidth - ContentPadH() - IconSize();
}

inline float CategoryGap() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space1);
}

inline float SectionRadius() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::CornerRadiusSmall);
}

inline float ToolbarIconSize() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::IconButtonSize)
        * WindEffects::Editor::UI::PanelChrome::UiScale();
}

inline float FilterButtonGap() {
    return WindEffects::Editor::UI::ResolveThemeMetric(WindEffects::Editor::UI::ThemeToken::Space2);
}

} // namespace we::programs::editor::ActorsPanelLayout
