#pragma once

#include "../Core/Widget.hpp"
#include "MenuBar.hpp"
#include "../Layout/Box.hpp"
#include <volk.h>
#include <SDL3/SDL.h>
#include <string>

namespace we::UI {

class TitleBar : public HorizontalBox {
public:
    TitleBar(SDL_Window* window, const std::string& title, VkDescriptorSet logoSet = VK_NULL_HANDLE, std::shared_ptr<MenuBar> menuBar = nullptr);
    virtual ~TitleBar() = default;

    void Construct() override;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    SDL_HitTestResult HitTest(SDL_Point point);

    SDL_Window* m_Window = nullptr;
    std::string m_Title;
    VkDescriptorSet m_LogoSet = VK_NULL_HANDLE;
    std::shared_ptr<MenuBar> m_MenuBar = nullptr;

    // Replaced explicit mock geometries with layout.
    // We only keep the HitTest implementation
    
    // For HitTest, we can cache specific widgets to check if they are being hovered
    std::shared_ptr<Widget> m_SearchWidget;
    std::shared_ptr<Widget> m_MinimizeWidget;
    std::shared_ptr<Widget> m_MaximizeWidget;
    std::shared_ptr<Widget> m_CloseWidget;
    std::vector<std::shared_ptr<Widget>> m_InteractableWidgets;
};

} // namespace we::editor::mainframe::UI
