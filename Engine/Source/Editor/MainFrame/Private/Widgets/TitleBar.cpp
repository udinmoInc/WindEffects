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

namespace we::UI {

TitleBar::TitleBar(SDL_Window* window, const std::string& title, VkDescriptorSet logoSet, std::shared_ptr<MenuBar> menuBar)
    : m_Window(window), m_Title(title), m_LogoSet(logoSet), m_MenuBar(menuBar)
{
    SetPadding(Margin{ 8.0f, 0.0f, 8.0f, 0.0f }); // 8px left and right margin
    SetSpacing(2.0f); // 2px gap between logo and MenuBar
}

void TitleBar::Construct() {

    // Left Container
    if (m_LogoSet != VK_NULL_HANDLE) {
        auto img = std::make_shared<Image>(m_LogoSet);
        img->SetSize(Size{ 20.0f, 20.0f });
        img->SetTintColor(Color{ 0.835f, 0.835f, 0.835f, 1.0f }); // #D5D5D5
        AddChild(img);
    } else {
        AddChild(std::make_shared<IconWidget>(Icons::CameraName, 20.0f));
    }
    
    if (m_MenuBar) {
        AddChild(m_MenuBar);
    } else {
        auto engineLabel = std::make_shared<Label>("WindEffects");
        engineLabel->SetStyle(TextStyle::Body());
        AddChild(engineLabel);
    }

    // Only one Spacer to push right controls to right
    AddChild(std::make_shared<Spacer>());


    // Right Container
    auto searchBtn = std::make_shared<ToolButton>(Icons::SearchName, "");
    auto gitBtn = std::make_shared<ToolButton>(Icons::UndoName, "");
    auto notifBtn = std::make_shared<ToolButton>(Icons::InfoName, "");
    auto settingsBtn = std::make_shared<ToolButton>(Icons::SettingsName, "");
    auto profileBtn = std::make_shared<ToolButton>(Icons::PropertiesName, ""); // fallback for user
    
    searchBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    gitBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    notifBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    settingsBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    profileBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    
    m_SearchWidget = searchBtn;

    auto toolbarBox = std::make_shared<HorizontalBox>();
    toolbarBox->SetSpacing(8.0f); // 8px gap between toolbar icons
    toolbarBox->AddChild(searchBtn);
    toolbarBox->AddChild(gitBtn);
    toolbarBox->AddChild(notifBtn);
    toolbarBox->AddChild(settingsBtn);
    toolbarBox->AddChild(profileBtn);

    auto minimizeBtn = std::make_shared<ToolButton>(Icons::MinimizeName, "");
    auto maximizeBtn = std::make_shared<ToolButton>(Icons::MaximizeName, "");
    auto closeBtn = std::make_shared<ToolButton>(Icons::XName, "");
    
    minimizeBtn->SetButtonStyle(ToolButtonStyle::WindowControl);
    maximizeBtn->SetButtonStyle(ToolButtonStyle::WindowControl);
    closeBtn->SetButtonStyle(ToolButtonStyle::WindowClose);
    
    m_MinimizeWidget = minimizeBtn;
    m_MaximizeWidget = maximizeBtn;
    m_CloseWidget = closeBtn;
    
    auto windowControls = std::make_shared<HorizontalBox>();
    windowControls->SetSpacing(8.0f); // 8px spacing for window controls
    windowControls->AddChild(m_MinimizeWidget);
    windowControls->AddChild(m_MaximizeWidget);
    windowControls->AddChild(m_CloseWidget);

    auto rightContainer = std::make_shared<HorizontalBox>();
    rightContainer->SetSpacing(20.0f); // 20px group spacing
    rightContainer->AddChild(toolbarBox);
    rightContainer->AddChild(windowControls);

    AddChild(rightContainer);

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
    m_DesiredSize = Size{ availableSize.width, 32.0f }; // Force 32px height
    return m_DesiredSize;
}

void TitleBar::Arrange(const Rect& allottedRect) {
    HorizontalBox::Arrange(allottedRect);
}

void TitleBar::Paint(PaintContext& context) {
    // Draw background
    context.DrawRect(m_Geometry, Theme::Get().HeaderBackground);
    
    // Draw centered title
    float textSize = Theme::Get().TextSizeWindow; // 13px
    float titleWidth = context.GetTextWidth(m_Title, textSize);
    float textX = m_Geometry.x + (m_Geometry.width - titleWidth) / 2.0f;
    float textY = m_Geometry.y + (m_Geometry.height - textSize) / 2.0f;
    context.DrawText(m_Title, Point{textX, textY}, Theme::Get().TextWindowLabel, textSize, true);
    
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

} // namespace we::editor::mainframe::UI
