#include "PropertyEditor/PropertyEditorSession.h"
#include "PropertyEditor/IDetailsView.h"
#include "PropertyEditorInternal.h"

#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Core/Widgets/PanelToolbarRow.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <string>
#include <utility>
#include <vector>

namespace we::editor::property {
namespace detail {
namespace {

using we::runtime::kindui::MouseButton;
using we::runtime::kindui::MouseEvent;
using we::runtime::kindui::PaintContext;
using we::runtime::kindui::PanelToolbarRow;
using we::runtime::kindui::Point;
using we::runtime::kindui::Rect;
using we::runtime::kindui::Size;
using we::runtime::kindui::Widget;
using ::we::runtime::kindui::kWindIconNone;
namespace Layout = we::runtime::kindui::LayoutMetrics;
namespace PanelChrome = we::runtime::kindui::PropertyPanelChrome;
using we::runtime::kindui::ResolveMetric;
using we::runtime::kindui::MetricToken;
using ::we::editor::panels::Panel;

[[nodiscard]] bool HasDetailsSelection(const IDetailsView* details) {
    return details && details->HasSelection();
}

class DetailsChromeRegionWidget : public Widget {
public:
    void SetDetails(IDetailsView* details) { m_Details = details; }

    void Tick(float deltaTime) override {
        Widget::Tick(deltaTime);
        SyncVisibility();
    }

protected:
    void SyncVisibility() {
        const bool active = HasDetailsSelection(m_Details);
        if (active == IsVisible()) {
            return;
        }
        SetVisible(active);
        InvalidateLayout();
    }

    IDetailsView* m_Details = nullptr;
};

class SelectedObjectHeaderWidget final : public DetailsChromeRegionWidget {
public:
    Size Measure(const Size& availableSize) override {
        if (!IsVisible()) {
            m_DesiredSize = Size{ availableSize.width, 0.0f };
            return m_DesiredSize;
        }
        m_DesiredSize = Size{ availableSize.width, Layout::PropertyObjectHeaderHeight() };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(PaintContext& context) override {
        if (!IsVisible() || !m_Details) {
            return;
        }
        const std::string& title = m_Details->GetObjectTitle();
        if (title.empty()) {
            return;
        }
        PanelChrome::PaintDetailsObjectHeader(
            context,
            m_Geometry,
            title,
            m_Details->GetObjectIcon());
    }

    void Tick(float deltaTime) override {
        DetailsChromeRegionWidget::Tick(deltaTime);
        if (!m_Details) {
            return;
        }
        const std::string title = m_Details->GetObjectTitle();
        const auto icon = m_Details->GetObjectIcon();
        if (title != m_LastTitle || icon.stem != m_LastIcon.stem || icon.sizePx != m_LastIcon.sizePx) {
            m_LastTitle = title;
            m_LastIcon = icon;
            InvalidatePaint();
        }
    }

private:
    std::string m_LastTitle;
    we::runtime::kindui::WindIconRef m_LastIcon = kWindIconNone;
};

class DetailsSearchRowWidget final : public DetailsChromeRegionWidget {
public:
    explicit DetailsSearchRowWidget(std::shared_ptr<PanelToolbarRow> search)
        : m_Search(std::move(search)) {}

    Size Measure(const Size& availableSize) override {
        if (!IsVisible() || !m_Search) {
            m_DesiredSize = Size{ availableSize.width, 0.0f };
            return m_DesiredSize;
        }
        m_DesiredSize = m_Search->Measure(availableSize);
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        if (m_Search && IsVisible()) {
            m_Search->Arrange(allottedRect);
        }
    }

    void Paint(PaintContext& context) override {
        if (m_Search && IsVisible()) {
            m_Search->Paint(context);
        }
    }

    std::shared_ptr<Widget> HitTestPoint(const Point& pos, const Rect* clip) override {
        if (!IsVisible() || !m_Search) {
            return nullptr;
        }
        return m_Search->HitTestPoint(pos, clip);
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (m_Search && IsVisible()) {
            m_Search->OnMouseDown(event);
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        if (m_Search && IsVisible()) {
            m_Search->OnMouseMove(event);
        }
    }

    void OnMouseUp(const MouseEvent& event) override {
        if (m_Search && IsVisible()) {
            m_Search->OnMouseUp(event);
        }
    }

private:
    std::shared_ptr<PanelToolbarRow> m_Search;
};

class CategoryFilterTabsWidget final : public DetailsChromeRegionWidget {
public:
    Size Measure(const Size& availableSize) override {
        if (!IsVisible()) {
            m_DesiredSize = Size{ availableSize.width, 0.0f };
            return m_DesiredSize;
        }
        m_DesiredSize = Size{ availableSize.width, Layout::PropertyCategoryTabRowHeight() };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        LayoutTabs();
    }

    void Paint(PaintContext& context) override {
        if (!IsVisible() || !m_Details) {
            return;
        }
        LayoutTabs();
        const std::string active = m_Details->GetActiveCategory();
        for (const auto& tab : m_Tabs) {
            const bool isActive = tab.label == "All" ? active.empty() : active == tab.label;
            PanelChrome::PaintCategoryTab(context, tab.rect, tab.label, isActive, m_HoveredTab == tab.label);
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        if (!IsVisible()) {
            return;
        }
        const std::string prev = m_HoveredTab;
        m_HoveredTab = TabAt(event.position);
        if (prev != m_HoveredTab) {
            InvalidatePaint();
        }
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (!IsVisible() || event.button != MouseButton::Left || !m_Details) {
            return;
        }
        const std::string tab = TabAt(event.position);
        if (tab.empty()) {
            return;
        }
        m_Details->SetActiveCategory(tab == "All" ? "" : tab);
        InvalidatePaint();
    }

    void Tick(float deltaTime) override {
        DetailsChromeRegionWidget::Tick(deltaTime);
        if (!m_Details || !IsVisible()) {
            return;
        }
        const auto categories = m_Details->GetCategoryNames();
        if (categories != m_LastCategories) {
            m_LastCategories = categories;
            InvalidateLayout();
            InvalidatePaint();
        }
    }

private:
    struct TabSlot {
        std::string label;
        Rect rect;
    };

    void LayoutTabs() {
        m_Tabs.clear();
        if (!m_Details || !IsVisible()) {
            return;
        }

        const float scale = std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
        const float padH = ResolveMetric(MetricToken::Space2) * scale;
        const float padV = ResolveMetric(MetricToken::Space1) * scale;
        const float tabH = PanelChrome::CategoryTabHeight();
        const float gap = ResolveMetric(MetricToken::Space1) * scale;
        const float minTabW = padH * 3.0f;

        float x = m_Geometry.x + padH;
        const float y = m_Geometry.y + padV;

        auto addTab = [&](const std::string& label, float textWidth) {
            const float tabW = std::max(minTabW, textWidth + padH * 2.0f);
            m_Tabs.push_back(TabSlot{ label, Rect{ x, y, tabW, tabH } });
            x += tabW + gap;
        };

        const float captionSize = ResolveMetric(MetricToken::TextSizeCaption) * scale;
        addTab("All", captionSize * 1.5f);
        for (const auto& category : m_Details->GetCategoryNames()) {
            addTab(category, static_cast<float>(category.size()) * captionSize * 0.55f);
        }
    }

    [[nodiscard]] std::string TabAt(const Point& pos) const {
        for (const auto& tab : m_Tabs) {
            if (tab.rect.Contains(pos)) {
                return tab.label;
            }
        }
        return {};
    }

    std::vector<TabSlot> m_Tabs;
    std::string m_HoveredTab;
    std::vector<std::string> m_LastCategories;
};

} // namespace

void PopulateDetailsPanelRegions(
    const std::shared_ptr<Panel>& panel,
    const std::shared_ptr<Widget>& propertyList,
    IDetailsView* details)
{
    auto header = std::make_shared<SelectedObjectHeaderWidget>();
    header->SetDetails(details);
    header->SetFlexShrink(0.0f);
    header->SetVisible(false);

    auto searchInner = std::make_shared<PanelToolbarRow>("Search...");
    searchInner->SetOnSearchChanged([details](const std::string& text) {
        if (details) {
            details->SetSearchText(text);
        }
    });
    searchInner->Finalize();
    searchInner->SetFlexShrink(0.0f);

    auto search = std::make_shared<DetailsSearchRowWidget>(searchInner);
    search->SetDetails(details);
    search->SetFlexShrink(0.0f);
    search->SetVisible(false);

    auto tabs = std::make_shared<CategoryFilterTabsWidget>();
    tabs->SetDetails(details);
    tabs->SetFlexShrink(0.0f);
    tabs->SetVisible(false);

    if (propertyList) {
        propertyList->SetFlexGrow(1.0f);
        propertyList->SetFlexShrink(1.0f);
    }

    panel->SetModeTabs(header);
    panel->SetSearch(search);
    panel->SetColumnHeader(tabs);
    panel->SetContent(propertyList);
}

} // namespace detail
} // namespace we::editor::property
