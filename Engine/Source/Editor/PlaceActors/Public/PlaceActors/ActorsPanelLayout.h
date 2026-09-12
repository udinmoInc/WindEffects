// ==============================================================================
// WindEffects — PlaceActors — ActorsPanelLayout
// Public API surface for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Rendering/IconMetrics.h"

namespace we::programs::editor::ActorsPanelLayout {

using MetricToken = we::runtime::kindui::MetricToken;

inline float ContentPadH() {
    return ::we::runtime::kindui::panels::PanelChrome::PanelPaddingH();
}

inline float ContentPadV() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space2);
}

inline float ChevronSize() {
    return static_cast<float>(16u);
}

inline float ActorRowHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::FormRowHeight)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

inline float CategoryHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CategoryHeaderHeight)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

inline float IconSize() {
    return 16.0f;
}

inline float RowRadius() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

inline float SearchHeight() {
    return ::we::runtime::kindui::panels::PanelChrome::SearchHeight();
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
    return we::runtime::kindui::ChromeSeparation::GapWide();
}

inline float CategoryContentGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space2);
}

inline float SectionRadius() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall);
}

inline float ToolbarIconSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::IconButtonSize)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

inline float FilterButtonGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space2);
}

inline float GridMinCardWidth() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ContentBrowserCellMedium)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

inline float GridMaxCardWidth() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ContentBrowserCellLarge)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

inline float GridCardGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ContentBrowserGridHSpacing);
}

inline float GridLabelHeight() {
    return we::runtime::kindui::ResolveMetric(MetricToken::TextSizeCaption)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale();
}

inline float GridLabelGap() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space1);
}

inline float GridScrollbarReserve() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ScrollbarWidth)
        * ::we::runtime::kindui::panels::PanelChrome::UiScale()
        + we::runtime::kindui::ResolveMetric(MetricToken::Space1);
}

}
