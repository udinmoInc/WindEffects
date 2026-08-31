#pragma once

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Rendering/IconMetrics.h"

namespace we::programs::editor::ActorsPanelLayout {

using MetricToken = we::runtime::kindui::MetricToken;

inline float ContentPadH() {
    return ::we::editor::panels::PanelChrome::PanelPaddingH();
}

inline float ContentPadV() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space2);
}

inline float ChevronSize() {
    return static_cast<float>(16u);
}

inline float ActorRowHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::FormRowHeight)
        * ::we::editor::panels::PanelChrome::UiScale();
}

inline float CategoryHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CategoryHeaderHeight)
        * ::we::editor::panels::PanelChrome::UiScale();
}

inline float IconSize() {
    return 16.0f;
}

inline float RowRadius() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall)
        * ::we::editor::panels::PanelChrome::UiScale();
}

inline float SearchHeight() {
    return ::we::editor::panels::PanelChrome::SearchHeight();
}

inline float SearchRowHeight() {
    return SearchHeight() + ContentPadV() * 2.0f
        + we::runtime::kindui::ResolveMetric(MetricToken::Space1);
}

inline float ItemIndent() {
    return ChevronSize() + we::runtime::kindui::ResolveMetric(MetricToken::Space2);
}

inline float ItemIconX(float contentX) {
    return contentX + ContentPadH() + ItemIndent();
}

inline float LabelX(float contentX) {
    return ItemIconX(contentX) + IconSize()
        + we::runtime::kindui::ResolveMetric(MetricToken::Space2);
}

inline float StarIconX(float contentX, float contentWidth) {
    return contentX + contentWidth - ContentPadH() - IconSize();
}

inline float CategoryGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::SectionGap);
}

inline float CategoryContentGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ContentGap);
}

inline float SectionRadius() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall);
}

inline float ToolbarIconSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::IconButtonSize)
        * ::we::editor::panels::PanelChrome::UiScale();
}

inline float FilterButtonGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space2);
}

inline float GridMinCardWidth() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ContentBrowserCellMedium)
        * ::we::editor::panels::PanelChrome::UiScale();
}

inline float GridMaxCardWidth() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ContentBrowserCellLarge)
        * ::we::editor::panels::PanelChrome::UiScale();
}

inline float GridCardGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ContentBrowserGridHSpacing);
}

inline float GridLabelHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TextSizeCaption)
        * ::we::editor::panels::PanelChrome::UiScale();
}

inline float GridLabelGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space1);
}

inline float GridScrollbarReserve() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ScrollbarWidth)
        * ::we::editor::panels::PanelChrome::UiScale()
        + we::runtime::kindui::ResolveMetric(MetricToken::Space1);
}

} // namespace we::programs::editor::ActorsPanelLayout
