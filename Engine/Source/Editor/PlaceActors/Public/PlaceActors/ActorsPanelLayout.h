#pragma once

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeToken.h"
#include "KindUI/Rendering/IconMetrics.h"

namespace we::programs::editor::ActorsPanelLayout {

inline float ContentPadH() {
    return we::runtime::kindui::PanelChrome::PanelPaddingH();
}

inline float ContentPadV() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space2);
}

inline float ChevronSize() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::IconSizeTree);
}

inline float ActorRowHeight() {
    return 32.0f * we::runtime::kindui::PanelChrome::UiScale();
}

inline float CategoryHeight() {
    return 28.0f * we::runtime::kindui::PanelChrome::UiScale();
}

inline float IconSize() {
    return static_cast<float>(we::runtime::kindui::IconMetrics::NativeIconTierPx(
        we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::IconSizeTree)));
}

inline float RowRadius() {
    return 5.0f * we::runtime::kindui::PanelChrome::UiScale();
}

inline float SearchHeight() {
    return 28.0f * we::runtime::kindui::PanelChrome::UiScale();
}

inline float SearchRowHeight() {
    return SearchHeight() + ContentPadV() * 2.0f + 4.0f;
}

inline float ItemIndent() {
    return ChevronSize() + we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space2);
}

inline float ItemIconX(float contentX) {
    return contentX + ContentPadH() + ItemIndent();
}

inline float LabelX(float contentX) {
    return ItemIconX(contentX) + IconSize()
        + we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space2);
}

inline float StarIconX(float contentX, float contentWidth) {
    return contentX + contentWidth - ContentPadH() - IconSize();
}

inline float CategoryGap() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space3);
}

inline float CategoryContentGap() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space2);
}

inline float SectionRadius() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::CornerRadiusSmall);
}

inline float ToolbarIconSize() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::IconButtonSize)
        * we::runtime::kindui::PanelChrome::UiScale();
}

inline float FilterButtonGap() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space2);
}

inline float GridMinCardWidth() {
    return 76.0f * we::runtime::kindui::PanelChrome::UiScale();
}

inline float GridMaxCardWidth() {
    return 112.0f * we::runtime::kindui::PanelChrome::UiScale();
}

inline float GridCardGap() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space3);
}

inline float GridLabelHeight() {
    return 16.0f * we::runtime::kindui::PanelChrome::UiScale();
}

inline float GridLabelGap() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space1);
}

// Always reserve scrollbar track width so the last column never sits under the thumb.
inline float GridScrollbarReserve() {
    return we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::ScrollbarWidth)
        * we::runtime::kindui::PanelChrome::UiScale()
        + we::runtime::kindui::ResolveThemeMetric(we::runtime::kindui::ThemeToken::Space1);
}

} // namespace we::programs::editor::ActorsPanelLayout
