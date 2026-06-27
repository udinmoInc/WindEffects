#include "TitleBar.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include "../Layout/Spacer.hpp"
#include "IconWidget.hpp"
#include "Label.hpp"
#include "ToolButton.hpp"
#include "SearchBox.hpp"
#include "Panel.hpp"
#include "Image.hpp"
#include <iostream>

namespace HouseEngine::UI {

TitleBar::TitleBar(SDL_Window* window, const std::string& title, VkDescriptorSet logoSet, std::shared_ptr<MenuBar> menuBar)
    : m_Window(window), m_Title(title), m_LogoSet(logoSet), m_MenuBar(menuBar)
{
    SetPadding(Margin{ 8.0f, 0.0f, 8.0f, 0.0f });
    SetSpacing(8.0f);
}

void TitleBar::Construct() {

    // Left Container
    if (m_LogoSet != VK_NULL_HANDLE) {
        auto img = std::make_shared<Image>(m_LogoSet);
        img->SetSize(Size{ 20.0f, 20.0f });
        img->SetTintColor(Theme::Get().TextPrimary);
        AddChild(img);
    } else {
        AddChild(std::make_shared<IconWidget>(Icons::CameraName, 20.0f));
    }
    
    if (m_MenuBar) {
        // We do not want MenuBar to be huge, just natural size
        AddChild(m_MenuBar);
    } else {
        auto engineLabel = std::make_shared<Label>("WindEffects");
        engineLabel->SetStyle(TextStyle::Body());
        AddChild(engineLabel);
    }

    // First Spacer to push Project Badge to center
    AddChild(std::make_shared<Spacer>());

    // Center Container (Project Badge)
    auto projectBadge = std::make_shared<Label>(m_Title);
    projectBadge->SetStyle(TextStyle::Small());
    AddChild(projectBadge);

    // Second Spacer to push right controls to right
    AddChild(std::make_shared<Spacer>());

    // Right Container
    m_SearchWidget = std::make_shared<SearchBox>();
    AddChild(m_SearchWidget);

    auto gitBtn = std::make_shared<ToolButton>(Icons::UndoName, "");
    auto notifBtn = std::make_shared<ToolButton>(Icons::InfoName, "");
    auto settingsBtn = std::make_shared<ToolButton>(Icons::SettingsName, "");
    auto profileBtn = std::make_shared<ToolButton>(Icons::PropertiesName, ""); // fallback for user
    
    AddChild(gitBtn);
    AddChild(notifBtn);
    AddChild(settingsBtn);
    AddChild(profileBtn);

    m_MinimizeWidget = std::make_shared<ToolButton>(Icons::MinimizeName, "");
    m_MaximizeWidget = std::make_shared<ToolButton>(Icons::MaximizeName, "");
    m_CloseWidget = std::make_shared<ToolButton>(Icons::XName, "");

    AddChild(m_MinimizeWidget);
    AddChild(m_MaximizeWidget);
    AddChild(m_CloseWidget);

    m_InteractableWidgets.push_back(m_SearchWidget);
    m_InteractableWidgets.push_back(gitBtn);
    m_InteractableWidgets.push_back(notifBtn);
    m_InteractableWidgets.push_back(settingsBtn);
    m_InteractableWidgets.push_back(profileBtn);
    m_InteractableWidgets.push_back(m_MinimizeWidget);
    m_InteractableWidgets.push_back(m_MaximizeWidget);
    m_InteractableWidgets.push_back(m_CloseWidget);
    
    if (m_MenuBar) {
        m_InteractableWidgets.push_back(m_MenuBar);
    }
}

Size TitleBar::Measure(const Size& availableSize) {
    HorizontalBox::Measure(availableSize);
    m_DesiredSize = Size{ availableSize.width, 36.0f }; // Force 36px height
    return m_DesiredSize;
}

void TitleBar::Arrange(const Rect& allottedRect) {
    HorizontalBox::Arrange(allottedRect);
}

void TitleBar::Paint(PaintContext& context) {
    // Draw background
    context.DrawRect(m_Geometry, Theme::Get().HeaderBackground);
    
    // Draw children
    HorizontalBox::Paint(context);
}

void TitleBar::OnMouseDown(const MouseEvent& event) {
    HorizontalBox::OnMouseDown(event);
}

void TitleBar::OnMouseMove(const MouseEvent& event) {
    HorizontalBox::OnMouseMove(event);
}

SDL_HitTestResult TitleBar::HitTest(SDL_Point point) {
    Point p{ (float)point.x, (float)point.y };
    
    // Check if clicking inside interactable widgets
    for (const auto& w : m_InteractableWidgets) {
        if (p.x >= w->GetGeometry().x && p.x <= w->GetGeometry().x + w->GetGeometry().width &&
            p.y >= w->GetGeometry().y && p.y <= w->GetGeometry().y + w->GetGeometry().height) {
            
            if (w == m_MinimizeWidget) return SDL_HITTEST_NORMAL;
            if (w == m_MaximizeWidget) return SDL_HITTEST_NORMAL;
            if (w == m_CloseWidget) return SDL_HITTEST_NORMAL;
            
            return SDL_HITTEST_NORMAL; // Search box and other buttons
        }
    }
    
    // If inside titlebar but not on a button, draggable
    if (p.x >= m_Geometry.x && p.x <= m_Geometry.x + m_Geometry.width &&
        p.y >= m_Geometry.y && p.y <= m_Geometry.y + m_Geometry.height) {
        return SDL_HITTEST_DRAGGABLE;
    }
    
    return SDL_HITTEST_NORMAL;
}

} // namespace HouseEngine::UI
