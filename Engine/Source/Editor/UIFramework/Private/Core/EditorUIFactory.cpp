#include "WindEffects/Editor/UI/Core/EditorUIFactory.h"

#include "Widgets/DropdownMenu.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::factory {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::ResolveMetric;
using ::we::runtime::kindui::ResolveColor;
using ::we::runtime::kindui::Row;
using ::we::editor::menus::DropdownMenu;

namespace {

class StandardVerticalDivider final : public Widget {
public:
    Size Measure(const Size& availableSize) override {
        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        m_DesiredSize = Size{ 1.0f * uiScale, availableSize.height };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
    }

    void Paint(PaintContext& context) override {
        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        const float w = 1.0f * uiScale;
        const float x = std::floor(m_Geometry.x + (m_Geometry.width - w) * 0.5f);
        context.DrawRect(Rect{ x, m_Geometry.y + 2.0f * uiScale, w, std::max(0.0f, m_Geometry.height - 4.0f * uiScale) }, ResolveColor(ColorToken::Separator));
    }
};

class StandardIconButton final : public Widget {
public:
    StandardIconButton(we::runtime::kindui::WindIconRef icon, std::function<void()> onClick, const std::string& tooltip)
        : m_Icon(icon), m_OnClick(std::move(onClick)), m_Tooltip(tooltip) {}

    Size Measure(const Size& availableSize) override {
        (void)availableSize;
        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        const float h = ResolveMetric(MetricToken::PanelHeaderHeight) * uiScale;
        m_DesiredSize = Size{ h, h };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(PaintContext& context) override {
        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        if (m_Hovered) {
            we::runtime::kindui::ControlChrome::InteractionState state{};
            state.hoverAnim = 1.0f;
            we::runtime::kindui::ControlChrome::PaintListRow(context, m_Geometry, state);
        }
        const float iconSz = 16.0f * uiScale;
        const Rect iconBand{
            m_Geometry.x + (m_Geometry.width - iconSz) * 0.5f,
            m_Geometry.y + (m_Geometry.height - iconSz) * 0.5f,
            iconSz, iconSz
        };
        IconPainter::Draw(context, m_Icon, iconBand, m_Hovered ? ResolveColor(ColorToken::TextPrimary) : ResolveColor(ColorToken::TextSecondary));
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Hovered = m_Geometry.Contains(event.position);
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (event.button == MouseButton::Left && m_OnClick) {
            m_OnClick();
        }
    }

    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

private:
    we::runtime::kindui::WindIconRef m_Icon;
    std::function<void()> m_OnClick;
    std::string m_Tooltip;
    bool m_Hovered = false;
};

} // namespace

std::shared_ptr<Widget> EditorUIFactory::CreateSearchInput(
    std::function<void(const std::string& query)> onQueryChanged,
    const std::string& placeholder)
{
    (void)placeholder;
    (void)onQueryChanged;
    return std::make_shared<StandardIconButton>(::we::runtime::kindui::WindIcons::Search16, nullptr, "Search");
}

std::shared_ptr<Widget> EditorUIFactory::CreateIconButton(
    we::runtime::kindui::WindIconRef icon,
    std::function<void()> onClick,
    const std::string& tooltip)
{
    return std::make_shared<StandardIconButton>(icon, std::move(onClick), tooltip);
}

std::shared_ptr<Widget> EditorUIFactory::CreateDropdownButton(
    const std::string& label,
    we::runtime::kindui::WindIconRef icon,
    std::vector<std::shared_ptr<we::editor::menus::MenuItem>> items,
    const std::string& tooltip)
{
    (void)label;
    auto itemsCopy = std::move(items);
    return std::make_shared<StandardIconButton>(icon, [itemsCopy = std::move(itemsCopy)]() {}, tooltip);
}

std::shared_ptr<Widget> EditorUIFactory::CreateVerticalDivider() {
    return std::make_shared<StandardVerticalDivider>();
}

} // namespace we::editor::factory
