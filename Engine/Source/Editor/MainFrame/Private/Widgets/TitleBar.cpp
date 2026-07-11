#include "Widgets/TitleBar.h"
#include "Widgets/MenuBar.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Widgets/Label.h"
#include "Widgets/Panel.h"
#include "Widgets/ToolButton.h"
#include "Core/DPIContext.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

namespace WindEffects::Editor::UI {

namespace {
    class FixedGap : public Widget {
    public:
        explicit FixedGap(float width) : m_Width(width) {}
        Size Measure(const Size& availableSize) override {
            m_DesiredSize = Size{ m_Width, 1.0f };
            return m_DesiredSize;
        }
        void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }
        void Paint(PaintContext& context) override {}
    private:
        float m_Width;
    };

    class LogoSlotWidget : public Widget {
    public:
        static constexpr float kSlotSize  = 32.0f;
        static constexpr float kIconSize  = kTitleBarLogoDisplaySize;

        explicit LogoSlotWidget(VkDescriptorSet logoSet) : m_LogoSet(logoSet) {}

        Size Measure(const Size& availableSize) override {
            m_DesiredSize = Size{ kSlotSize, kSlotSize };
            return m_DesiredSize;
        }
        void Arrange(const Rect& allottedRect) override {
            m_Geometry = allottedRect;
            if (allottedRect.height > kSlotSize) {
                m_Geometry.y += (allottedRect.height - kSlotSize) * 0.5f;
                m_Geometry.height = kSlotSize;
            }
        }
        void Paint(PaintContext& context) override {
            const float cx = m_Geometry.x + m_Geometry.width  * 0.5f;
            const float cy = m_Geometry.y + m_Geometry.height * 0.5f;
            const float half = kIconSize * 0.5f;
            const auto snap = [](float v) { return std::floor(v + 0.5f); };
            Rect logoRect{
                snap(cx - half),
                snap(cy - half),
                kIconSize,
                kIconSize
            };

            if (m_LogoSet != VK_NULL_HANDLE) {
                context.DrawTexture(logoRect, m_LogoSet, ThemeColor(ThemeToken::TextSecondary));
            } else {
                IconPainter::DrawIcon(context, Icons::CameraName, logoRect, ThemeColor(ThemeToken::AccentPrimary));
            }
        }
    private:
        VkDescriptorSet m_LogoSet;
    };

    class ProjectSelectorWidget : public Widget {
    public:
        static constexpr float kHeight      = 24.0f;
        static constexpr float kLeftPad     = 12.0f - 2.0f;
        static constexpr float kRightPad    = 12.0f - 2.0f;
        static constexpr float kIconSize    = 14.0f;
        static constexpr float kIconGap     = 8.0f - 2.0f;
        static constexpr float kChevSize    = 8.0f;
        static constexpr float kTextSize    = 12.0f;
        static constexpr const char* kProjectName = "WindEffects";

        ProjectSelectorWidget() {}
        Size Measure(const Size& availableSize) override {
            float textW = kProjectName[0] ? static_cast<float>(strlen(kProjectName)) * 7.2f : 0.0f;
            float width = kLeftPad + kIconSize + kIconGap + textW + 4.0f + kChevSize + kRightPad;
            m_DesiredSize = Size{ width, kHeight };
            return m_DesiredSize;
        }
        void Arrange(const Rect& allottedRect) override {
            m_Geometry = allottedRect;
            if (allottedRect.height > m_DesiredSize.height) {
                m_Geometry.y += (allottedRect.height - m_DesiredSize.height) / 2.0f;
                m_Geometry.height = m_DesiredSize.height;
            }
        }
        void Paint(PaintContext& context) override {
            Color bg = m_Hovered ? ThemeColor(ThemeToken::HoverBackground) : ThemeColor(ThemeToken::PanelBackground);
            context.DrawRoundedRect(m_Geometry, bg, ThemeMetric(ThemeToken::CornerRadiusSmall));

            Color borderCol = m_Hovered ? ThemeColor(ThemeToken::BorderLight) : ThemeColor(ThemeToken::BorderDefault);
            context.DrawRoundedRectOutline(m_Geometry, borderCol, ThemeMetric(ThemeToken::BorderWidth), ThemeMetric(ThemeToken::CornerRadiusSmall));

            float centerY = m_Geometry.y + m_Geometry.height / 2.0f;

            IconPainter::DrawIcon(context, Icons::PackageName,
                Rect{ m_Geometry.x + kLeftPad, centerY - kIconSize / 2.0f, kIconSize, kIconSize },
                ThemeColor(ThemeToken::TextSecondary));

            float textX = m_Geometry.x + kLeftPad + kIconSize + kIconGap;
            context.DrawText(kProjectName,
                Point{ textX, centerY - kTextSize / 2.0f },
                ThemeColor(ThemeToken::TextPrimary), kTextSize);

            float chevX = m_Geometry.x + m_Geometry.width - kRightPad - kChevSize;
            IconPainter::DrawIcon(context, Icons::ChevronDownName,
                Rect{ chevX, centerY - kChevSize / 2.0f, kChevSize, kChevSize },
                Color{ 0.549f, 0.549f, 0.549f, 1.0f });
        }
        void OnMouseMove(const MouseEvent& event) override { }
        void OnMouseDown(const MouseEvent& event) override { }
    };

    constexpr float kTitleBarHeight   = 40.0f;
    constexpr float kWindowPadLeft    = 8.0f;
    constexpr float kLogoToMenuGap    = 2.0f;
}

TitleBar::TitleBar(SDL_Window* window, const std::string& title, VkDescriptorSet logoSet, std::shared_ptr<MenuBar> menuBar)
    : m_Window(window), m_Title(title), m_LogoSet(logoSet), m_MenuBar(menuBar)
{
    SetPadding(Margin{ 0.0f, 0.0f, 6.0f, 0.0f });
    SetSpacing(0.0f);
}

void TitleBar::Construct() {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_LeftContainer = std::make_shared<HorizontalBox>();
    m_LeftContainer->SetSpacing(0.0f);
    
    m_LogoWidget = std::make_shared<LogoSlotWidget>(m_LogoSet);
    m_LeftContainer->AddChild(m_LogoWidget);
    m_LeftContainer->AddChild(std::make_shared<FixedGap>(kLogoToMenuGap * uiScale));
    
    if (m_MenuBar) {
        m_MenuBar->SetHeight(kTitleBarHeight * uiScale);
        m_LeftContainer->AddChild(m_MenuBar);
    }

    m_CenterContainer = std::make_shared<HorizontalBox>();
    m_CenterContainer->SetSpacing(0.0f);
    
    auto projectSelector = std::make_shared<ProjectSelectorWidget>();
    m_CenterContainer->AddChild(projectSelector);

    m_RightContainer = std::make_shared<HorizontalBox>();
    m_RightContainer->SetSpacing(0.0f);
    
    auto undoBtn  = std::make_shared<ToolButton>(Icons::UndoName, "");
    auto redoBtn  = std::make_shared<ToolButton>(Icons::RedoName, "");
    auto notifBtn = std::make_shared<ToolButton>(Icons::InfoName, "");
    
    undoBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    redoBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    notifBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);

    m_RightContainer->AddChild(undoBtn);
    m_RightContainer->AddChild(std::make_shared<FixedGap>(6.0f * uiScale));
    m_RightContainer->AddChild(redoBtn);
    m_RightContainer->AddChild(std::make_shared<FixedGap>(6.0f * uiScale));
    m_RightContainer->AddChild(notifBtn);
    m_RightContainer->AddChild(std::make_shared<FixedGap>(8.0f * uiScale));

    auto minimizeBtn = std::make_shared<ToolButton>(Icons::MinimizeName, "", [this]() {
        if (m_Window) SDL_MinimizeWindow(m_Window);
    });
    auto maximizeBtn = std::make_shared<ToolButton>(Icons::StopName, "", [this]() {
        if (m_Window) {
            auto flags = SDL_GetWindowFlags(m_Window);
            if (flags & SDL_WINDOW_MAXIMIZED) {
                SDL_RestoreWindow(m_Window);
            } else {
                SDL_MaximizeWindow(m_Window);
            }
            UpdateMaximizeIcon();
        }
    });
    auto closeBtn = std::make_shared<ToolButton>(Icons::XName, "", [this]() {
        if (m_Window) {
            SDL_Event event;
            event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            event.window.windowID = SDL_GetWindowID(m_Window);
            SDL_PushEvent(&event);
        }
    });
    
    minimizeBtn->SetButtonStyle(ToolButtonStyle::WindowControl);
    maximizeBtn->SetButtonStyle(ToolButtonStyle::WindowControl);
    closeBtn->SetButtonStyle(ToolButtonStyle::WindowClose);
    
    m_MinimizeWidget = minimizeBtn;
    m_MaximizeWidget = maximizeBtn;
    m_CloseWidget = closeBtn;
    
    m_RightContainer->AddChild(m_MinimizeWidget);
    m_RightContainer->AddChild(std::make_shared<FixedGap>(1.0f * uiScale));
    m_RightContainer->AddChild(m_MaximizeWidget);
    m_RightContainer->AddChild(std::make_shared<FixedGap>(1.0f * uiScale));
    m_RightContainer->AddChild(m_CloseWidget);

    // Update maximize icon based on initial window state
    UpdateMaximizeIcon();

    AddChild(m_LeftContainer);
    AddChild(m_CenterContainer);
    AddChild(m_RightContainer);

    m_InteractableWidgets.push_back(projectSelector);
    m_InteractableWidgets.push_back(undoBtn);
    m_InteractableWidgets.push_back(redoBtn);
    m_InteractableWidgets.push_back(notifBtn);
    m_InteractableWidgets.push_back(m_MinimizeWidget);
    m_InteractableWidgets.push_back(m_MaximizeWidget);
    m_InteractableWidgets.push_back(m_CloseWidget);
    
    if (m_MenuBar) {
        m_InteractableWidgets.push_back(m_MenuBar);
    }
}

Size TitleBar::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    if (m_LeftContainer) m_LeftContainer->Measure(availableSize);
    if (m_CenterContainer) m_CenterContainer->Measure(availableSize);
    if (m_RightContainer) m_RightContainer->Measure(availableSize);

    m_DesiredSize = Size{ availableSize.width, kTitleBarHeight * uiScale };
    return m_DesiredSize;
}

void TitleBar::Arrange(const Rect& allottedRect) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_Geometry = allottedRect;

    if (m_LeftContainer) {
        Size leftSize = m_LeftContainer->GetDesiredSize();
        m_LeftContainer->Arrange(Rect{
            allottedRect.x + kWindowPadLeft * uiScale,
            allottedRect.y,
            leftSize.width,
            allottedRect.height
        });
    }

    if (m_RightContainer) {
        Size rightSize = m_RightContainer->GetDesiredSize();
        m_RightContainer->Arrange(Rect{ allottedRect.x + allottedRect.width - rightSize.width - 6.0f * uiScale, allottedRect.y, rightSize.width, allottedRect.height });
    }

    if (m_CenterContainer) {
        Size centerSize = m_CenterContainer->GetDesiredSize();
        float centerX = allottedRect.x + (allottedRect.width - centerSize.width) / 2.0f;
        m_CenterContainer->Arrange(Rect{ centerX, allottedRect.y, centerSize.width, allottedRect.height });
    }
}

void TitleBar::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, ThemeColor(ThemeToken::HeaderBackground));

    context.DrawLine(
        Point{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f },
        Point{ m_Geometry.x + m_Geometry.width, m_Geometry.y + m_Geometry.height - 1.0f },
        ThemeColor(ThemeToken::BorderDark),
        1.0f);

    HorizontalBox::Paint(context);
}

void TitleBar::OnMouseDown(const MouseEvent& event) {
    HorizontalBox::OnMouseDown(event);
}

void TitleBar::OnMouseMove(const MouseEvent& event) {
    HorizontalBox::OnMouseMove(event);
}

void TitleBar::UpdateMaximizeIcon() {
    if (!m_Window || !m_MaximizeWidget) return;
    
    // Always use square icon for maximize button
    auto toolBtn = std::static_pointer_cast<ToolButton>(m_MaximizeWidget);
    toolBtn->SetIcon(Icons::StopName);
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
