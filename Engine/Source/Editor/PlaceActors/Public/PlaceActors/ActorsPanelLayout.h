#pragma once

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Rendering/IconMetrics.h"

namespace we::programs::editor::ActorsPanelLayout {

inline float ContentPadH() {
    return ::we::editor::panels::PanelChrome::PanelPaddingH();
}

inline float ContentPadV() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space2);
}

inline float ChevronSize() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::IconSizeTree);
}

inline float ActorRowHeight() {
    return 32.0f * ::we::editor::panels::PanelChrome::UiScale();
}

inline float CategoryHeight() {
    return 28.0f * ::we::editor::panels::PanelChrome::UiScale();
}

inline float IconSize() {
    return static_cast<float>(we::runtime::kindui::IconMetrics::NativeIconTierPx(
        we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::IconSizeTree)));
}

inline float RowRadius() {
    return 5.0f * ::we::editor::panels::PanelChrome::UiScale();
}

inline float SearchHeight() {
    return 28.0f * ::we::editor::panels::PanelChrome::UiScale();
}

inline float SearchRowHeight() {
    return SearchHeight() + ContentPadV() * 2.0f + 4.0f;
}

inline float ItemIndent() {
    return ChevronSize() + we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space2);
}

inline float ItemIconX(float contentX) {
    return contentX + ContentPadH() + ItemIndent();
}

inline float LabelX(float contentX) {
    return ItemIconX(contentX) + IconSize()
        + we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space2);
}

inline float StarIconX(float contentX, float contentWidth) {
    return contentX + contentWidth - ContentPadH() - IconSize();
}

inline float CategoryGap() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space3);
}

inline float CategoryContentGap() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space2);
}

inline float SectionRadius() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::CornerRadiusSmall);
}

inline float ToolbarIconSize() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::IconButtonSize)
        * ::we::editor::panels::PanelChrome::UiScale();
}

inline float FilterButtonGap() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space2);
}

inline float GridMinCardWidth() {
    return 76.0f * ::we::editor::panels::PanelChrome::UiScale();
}

inline float GridMaxCardWidth() {
    return 112.0f * ::we::editor::panels::PanelChrome::UiScale();
}

inline float GridCardGap() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space3);
}

inline float GridLabelHeight() {
    return 16.0f * ::we::editor::panels::PanelChrome::UiScale();
}

inline float GridLabelGap() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space1);
}

// Always reserve scrollbar track width so the last column never sits under the thumb.
inline float GridScrollbarReserve() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::ScrollbarWidth)
        * ::we::editor::panels::PanelChrome::UiScale()
        + we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space1);
}

} // namespace we::programs::editor::ActorsPanelLayout
