#include "ContentBrowser/Widgets/TreeColumnHeader.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "Text/Layout/TextStyle.h"

namespace we::editor::contentbrowser {
namespace Chrome = ::we::editor::panels::PanelChrome;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

using ::we::runtime::kindui::ColorToken;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;

Size TreeColumnHeader::Measure(const Size& availableSize) {
    m_DesiredSize = Size{
        availableSize.width < 1.0e8f ? availableSize.width : 0.0f,
        Chrome::ColumnHeaderRowHeight()
    };
    return m_DesiredSize;
}

void TreeColumnHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TreeColumnHeader::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float glyphTier = static_cast<float>(16u);
    const float headerTextSize = ThemeMetric(MetricToken::TextSizeCaption) * uiScale;
    const float headerTextY = m_Geometry.y + (m_Geometry.height - headerTextSize) * 0.5f;

    const float pad = ThemeMetric(MetricToken::Space2) * uiScale;
    const float hit = ThemeMetric(MetricToken::TreeExpanderHitSize) * uiScale;
    const float prefix = (ThemeMetric(MetricToken::Space2) + 2.0f * ThemeMetric(MetricToken::TreeExpanderHitSize)) * uiScale;
    const float eyeX = m_Geometry.x + pad;
    Rect eyeBand{ eyeX, m_Geometry.y, glyphTier, m_Geometry.height };
    IconPainter::Draw(
        context, WindIcons::Eye16, IconMetrics::PlaceGlyphCentered(eyeBand, 16u));

    const float lockX = m_Geometry.x + pad + hit;
    Rect lockBand{ lockX, m_Geometry.y, glyphTier, m_Geometry.height };
    IconPainter::Draw(
        context, WindIcons::LockOpen16, IconMetrics::PlaceGlyphCentered(lockBand, 16u));

    const float labelX = m_Geometry.x + prefix;
    context.DrawText(
        "Item Label",
        Point{ labelX, headerTextY },
        ThemeColor(ColorToken::TextSecondary),
        headerTextSize,
        we::runtime::text::layout::FontWeight::Medium);

    const float typeRightX = m_Geometry.x + m_Geometry.width - ThemeMetric(MetricToken::Space3) * uiScale;
    const float typeColumnReserve = ThemeMetric(MetricToken::Space6) * uiScale;
    const float dividerX = m_Geometry.x + m_Geometry.width - typeColumnReserve;
    const float sepH = ThemeMetric(MetricToken::ToolbarSeparatorHeight) * uiScale;
    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
    ControlChrome::PaintVerticalSeparator(
        context,
        dividerX,
        centerY - sepH * 0.5f,
        centerY + sepH * 0.5f,
        ThemeMetric(MetricToken::BorderWidth),
        ColorToken::Separator);

    const float typeWidth = context.GetTextWidth(
        "Type",
        headerTextSize,
        we::runtime::text::layout::FontWeight::Medium);
    context.DrawText(
        "Type",
        Point{ typeRightX - typeWidth, headerTextY },
        ThemeColor(ColorToken::TextSecondary),
        headerTextSize,
        we::runtime::text::layout::FontWeight::Medium);
}

} // namespace we::editor::contentbrowser
