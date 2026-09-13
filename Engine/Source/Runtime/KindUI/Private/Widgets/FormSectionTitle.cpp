// ==============================================================================
// WindEffects — KindUI — FormSectionTitle
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/FormSectionTitle.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Tokens/DesignToken.h"
#include "Text/Layout/TextStyle.h"

#include <algorithm>

namespace we::runtime::kindui {
namespace PanelChrome = we::runtime::kindui::panels::PanelChrome;

FormSectionTitle::FormSectionTitle(std::string title, bool leadingGap)
    : m_Title(std::move(title))
    , m_LeadingGap(leadingGap) {}

void FormSectionTitle::SetTitle(std::string title) {
    if (m_Title == title) {
        return;
    }
    m_Title = std::move(title);
    InvalidatePaint();
}

void FormSectionTitle::SetLeadingGap(bool leadingGap) {
    if (m_LeadingGap == leadingGap) {
        return;
    }
    m_LeadingGap = leadingGap;
    InvalidateLayout();
}

Size FormSectionTitle::Measure(const Size& availableSize) {
    const float gap = m_LeadingGap ? ChromeSeparation::GapWide() : 0.0f;
    const float bandH = LayoutMetrics::UnifiedSectionHeaderHeight();
    m_DesiredSize = Size{ availableSize.width, gap + bandH };
    return m_DesiredSize;
}

void FormSectionTitle::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float gap = m_LeadingGap ? ChromeSeparation::GapWide() : 0.0f;
    m_TitleBand = Rect{
        allottedRect.x,
        allottedRect.y + gap,
        allottedRect.width,
        std::max(0.0f, allottedRect.height - gap)
    };
}

void FormSectionTitle::Paint(PaintContext& context) {
    if (m_TitleBand.IsEmpty()) {
        return;
    }
    PanelChrome::PaintListLabelBand(context, m_TitleBand);
    const float scale = std::max(1.0f, DPIContext::GetScale());
    const float fontSize = ResolveMetric(MetricToken::TextSizeCategory) * scale;
    const float textY = LayoutMetrics::AlignTextTopY(m_TitleBand, fontSize);
    context.DrawText(
        m_Title,
        Point{ m_TitleBand.x, textY },
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        we::runtime::text::layout::FontWeight::Regular);
}

} // namespace we::runtime::kindui
