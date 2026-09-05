#include "ContentBrowser/Widgets/TreeColumnHeader.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "Text/Layout/TextStyle.h"

namespace we::editor::contentbrowser {
namespace Chrome = ::we::editor::panels::PanelChrome;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace LayoutMetrics = ::we::runtime::kindui::LayoutMetrics;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::Color;
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
    const float headerTextSize = ThemeMetric(MetricToken::TextSizeCaption) * uiScale;
    const float headerTextY = LayoutMetrics::AlignTextTopY(m_Geometry, headerTextSize);
    const Color sepColor = ThemeColor(ColorToken::Separator);
    const Color textColor = ThemeColor(ColorToken::TextSecondary);

    const float borderW = std::max(1.0f, ThemeMetric(MetricToken::BorderWidth));
    const float topBorderY = std::floor(m_Geometry.y);
    const float botBorderY = std::floor(m_Geometry.y + m_Geometry.height - borderW);

    // Top & Bottom horizontal border lines
    context.DrawRect(Rect{ m_Geometry.x, topBorderY, m_Geometry.width, borderW }, sepColor);
    context.DrawRect(Rect{ m_Geometry.x, botBorderY, m_Geometry.width, borderW }, sepColor);

    // Column 0: Eye column (spacious 30px cell)
    const float eyeColWidth = std::floor(30.0f * uiScale);
    Rect eyeBand{ m_Geometry.x, m_Geometry.y, eyeColWidth, m_Geometry.height };
    IconPainter::Draw(
        context, WindIcons::Eye16, IconMetrics::PlaceGlyphCentered(eyeBand, 16u), textColor);

    // Vertical Separator after Eye column
    const float sep1X = std::floor(m_Geometry.x + eyeColWidth);
    context.DrawRect(Rect{ sep1X, m_Geometry.y, borderW, m_Geometry.height }, sepColor);

    // Column 1: Star / Dirty column (spacious 28px cell with crisp 16u Star icon)
    const float dirtyColWidth = std::floor(28.0f * uiScale);
    const float sep2X = std::floor(sep1X + dirtyColWidth);
    Rect starBand{ sep1X, m_Geometry.y, dirtyColWidth, m_Geometry.height };
    IconPainter::Draw(
        context, WindIcons::Star16, IconMetrics::PlaceGlyphCentered(starBand, 16u), textColor);

    // Vertical Separator after Star column
    context.DrawRect(Rect{ sep2X, m_Geometry.y, borderW, m_Geometry.height }, sepColor);

    // Column 2: Item Label (generous 16px gap after separator)
    const float labelPad = std::floor(16.0f * uiScale);
    const float labelX = sep2X + labelPad;
    context.DrawText(
        "Item Label",
        Point{ labelX, headerTextY },
        textColor,
        headerTextSize,
        we::runtime::text::layout::FontWeight::Medium);

    // Column 3: Type column
    const float typeColWidth = std::floor(90.0f * uiScale);
    const float sep3X = std::floor(m_Geometry.x + m_Geometry.width - typeColWidth);
    context.DrawRect(Rect{ sep3X, m_Geometry.y, borderW, m_Geometry.height }, sepColor);

    const float typeX = sep3X + labelPad;
    context.DrawText(
        "Type",
        Point{ typeX, headerTextY },
        textColor,
        headerTextSize,
        we::runtime::text::layout::FontWeight::Medium);
}

} // namespace we::editor::contentbrowser
