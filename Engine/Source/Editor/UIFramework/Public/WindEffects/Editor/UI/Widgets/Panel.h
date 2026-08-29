#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/Icon.h"
#include "WindEffects/Editor/UI/Panel/PanelBodyLayout.h"
#include "RHI/Types.h"
#include <string>
#include <memory>
#include <functional>

namespace we::editor::panels {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;

// Panel widget with collapsible header and content area
class UIFRAMEWORK_API Panel : public Widget {
public:
    Panel(const std::string& title = "");
    virtual ~Panel() = default;

    /// Attach the body layout after the panel is owned by std::shared_ptr.
    void AttachBodyLayout();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    [[nodiscard]] std::shared_ptr<Widget> HitTestPoint(const Point& pos, const Rect* clip = nullptr) override;
    [[nodiscard]] bool IsInteractiveContainer() const override { return true; }

    // Content management
    void SetContent(const std::shared_ptr<Widget>& content);
    std::shared_ptr<Widget> GetContent() const;

    // Body region management (shared vertical hierarchy)
    void SetModeTabs(const std::shared_ptr<Widget>& modeTabs);
    void SetSearch(const std::shared_ptr<Widget>& search);
    void SetColumnHeader(const std::shared_ptr<Widget>& columnHeader);
    void SetFooter(const std::shared_ptr<Widget>& footer);
    [[nodiscard]] std::shared_ptr<PanelBodyLayout> GetBodyLayout() const { return m_BodyLayout; }
    [[nodiscard]] Rect GetRegionRect(PanelBodyRegion region) const;
    
    // Toolbar management
    void SetToolbar(const std::shared_ptr<Widget>& toolbar);
    std::shared_ptr<Widget> GetToolbar() const;

    // Header management
    void SetTitle(const std::string& title) { m_Title = title; }
    std::string GetTitle() const { return m_Title; }

    // Collapse state
    void SetExpanded(bool expanded);
    bool IsExpanded() const { return m_Expanded; }
    void Toggle() { SetExpanded(!m_Expanded); }

    // Header actions (icons on the right side of header)
    void AddHeaderAction(const std::string& iconName, std::function<void()> onClick);

    void SetOptionsMenuHandler(std::function<void()> onClick) { m_OnOptionsMenu = std::move(onClick); }
    void InvokeOptionsMenu() const;
    bool HasOptionsMenuHandler() const { return static_cast<bool>(m_OnOptionsMenu); }

    // Styling
    void SetHeaderHeight(float height) { m_HeaderHeight = height; }
    void SetCollapsible(bool collapsible) { m_Collapsible = collapsible; }
    void SetBackgroundColor(const Color& color) { m_Style.background.color = color; }
    void SetStyle(const WidgetStyle& style) { m_Style = style; }
    void SetTransparentBackground(bool transparent) {
        m_TransparentBackground = transparent;
        if (m_BodyLayout) {
            m_BodyLayout->SetSuppressContentSurfaces(transparent);
        }
    }
    bool IsTransparentBackground() const { return m_TransparentBackground; }
    void SetFloatingToolbar(bool floating) {
        m_FloatingToolbar = floating;
        if (m_BodyLayout) {
            m_BodyLayout->SetOverlayToolbar(floating);
        }
    }
    bool IsFloatingToolbar() const { return m_FloatingToolbar; }

    void SetTabIcon(const std::string& iconName) { m_TabIconName = iconName; }
    [[nodiscard]] const std::string& GetTabIcon() const { return m_TabIconName; }

    void SetTabBrand(we::rhi::RHIDescriptorSetHandle descriptor, float logicalSize);
    [[nodiscard]] bool HasTabBrand() const { return m_TabBrandDescriptor != we::rhi::RHIDescriptorSetHandle::Invalid; }
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle GetTabBrandDescriptor() const { return m_TabBrandDescriptor; }
    [[nodiscard]] float GetTabBrandLogicalSize() const { return m_TabBrandLogicalSize; }

private:
    struct HeaderAction {
        std::string iconName;
        std::function<void()> onClick;
        Rect geometry;
    };

    void CalculateHeaderGeometries();
    HeaderAction* GetActionAtPosition(const Point& pos);

    std::string m_Title;
    std::shared_ptr<PanelBodyLayout> m_BodyLayout;
    std::vector<HeaderAction> m_HeaderActions;
    std::function<void()> m_OnOptionsMenu;

    bool m_Expanded = true;
    bool m_Collapsible = true;
    bool m_HeaderHovered = false;
    bool m_OptionsMenuHovered = false;
    bool m_OptionsMenuPressed = false;
    int m_HoveredActionIndex = -1;
    int m_PressedActionIndex = -1;
    bool m_TransparentBackground = false;
    bool m_FloatingToolbar = false;

    float m_HeaderHeight = 0.0f; // resolved in constructor from PanelTabHeight token
    float m_ActionIconSize = 16.0f;
    float m_ActionSpacing = 4.0f;

    Rect m_HeaderRect;
    Rect m_OptionsMenuRect;

    WidgetStyle m_Style;
    WidgetStyle m_HeaderStyle;

    std::string m_TabIconName;
    we::rhi::RHIDescriptorSetHandle m_TabBrandDescriptor = we::rhi::RHIDescriptorSetHandle::Invalid;
    float m_TabBrandLogicalSize = 0.0f;
};

} // namespace we::editor::panels
