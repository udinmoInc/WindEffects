#pragma once

#include "MainFrame/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Layout/Flex.h"
#include "Platform/Types.h"
#include "RHI/Types.h"

#include <string>

namespace we::editor::menus { class MenuBar; }
namespace we::editor::toolbar { class ToolButton; }

namespace we::editor::shell {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;
using ::we::runtime::kindui::Row;

inline constexpr float kTitleBarHeight          = 34.0f;
inline constexpr float kTitleBarLogoDisplaySize = 18.0f;
inline constexpr float kHeaderControlHeight     = 24.0f;
inline constexpr float kWindowControlWidth      = 40.0f;
inline constexpr float kWindowControlCount      = 3.0f;

class MAINFRAME_API TitleBar : public Row {
public:
    TitleBar(we::platform::WindowId window, const std::string& title,
             we::rhi::RHIDescriptorSetHandle logoSet = we::rhi::RHIDescriptorSetHandle::Invalid,
             std::shared_ptr<::we::editor::menus::MenuBar> menuBar = nullptr);
    virtual ~TitleBar() = default;

    void Construct() override;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    we::platform::WindowHitTestResult HitTest(we::platform::Int2 point);
    void UpdateMaximizeIcon();

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    std::string m_Title;
    we::rhi::RHIDescriptorSetHandle m_LogoSet = we::rhi::RHIDescriptorSetHandle::Invalid;
    std::shared_ptr<::we::editor::menus::MenuBar> m_MenuBar = nullptr;

    std::shared_ptr<Widget> m_LogoWidget;
    std::shared_ptr<Widget> m_MinimizeWidget;
    std::shared_ptr<Widget> m_MaximizeWidget;
    std::shared_ptr<Widget> m_CloseWidget;
    std::vector<std::shared_ptr<Widget>> m_InteractableWidgets;

    std::shared_ptr<Row> m_LeftContainer;
    std::shared_ptr<Row> m_CenterContainer;
    std::shared_ptr<Row> m_RightContainer;
};

} // namespace we::editor::shell