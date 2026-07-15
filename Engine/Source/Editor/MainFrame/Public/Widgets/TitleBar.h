#pragma once

#include "MainFrame/Export.h"

#include "Core/Widget.h"
#include "Platform/Types.h"
namespace WindEffects::Editor::UI { class MenuBar; class ToolButton; }
#include "Layout/Box.h"
#include <volk.h>
#include <string>

namespace WindEffects::Editor::UI {

inline constexpr float kTitleBarHeight          = 34.0f;
inline constexpr float kTitleBarLogoDisplaySize = 18.0f;
inline constexpr float kHeaderControlHeight     = 24.0f;
inline constexpr float kWindowControlWidth      = 40.0f;
inline constexpr float kWindowControlCount      = 3.0f;

class MAINFRAME_API TitleBar : public HorizontalBox {
public:
    TitleBar(we::platform::WindowId window, const std::string& title, VkDescriptorSet logoSet = VK_NULL_HANDLE, std::shared_ptr<MenuBar> menuBar = nullptr);
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
    VkDescriptorSet m_LogoSet = VK_NULL_HANDLE;
    std::shared_ptr<MenuBar> m_MenuBar = nullptr;

    std::shared_ptr<Widget> m_LogoWidget;
    std::shared_ptr<Widget> m_MinimizeWidget;
    std::shared_ptr<Widget> m_MaximizeWidget;
    std::shared_ptr<Widget> m_CloseWidget;
    std::vector<std::shared_ptr<Widget>> m_InteractableWidgets;

    std::shared_ptr<HorizontalBox> m_LeftContainer;
    std::shared_ptr<HorizontalBox> m_CenterContainer;
    std::shared_ptr<HorizontalBox> m_RightContainer;
};

} // namespace WindEffects::Editor::UI
