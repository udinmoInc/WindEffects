// ==============================================================================
// WindEffects — MainFrame — TitleBar
// UI widget used by the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Widgets/TitleBar.h"
#include "Widgets/MenuBar.h"
#include <KindUI/EditorUI.h>
#include "Widgets/ToolButton.h"
#include "Platform/Platform.h"
#include "Core/LoopExecutionTrace.h"
#include "Projects/ProjectContext.h"
#include "Projects/EngineContext.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::IconColorRole;
using ::we::runtime::kindui::ResolveIconColor;
using ::we::runtime::kindui::Margin;
using ::we::runtime::kindui::Widget;

namespace we::editor::shell {
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::Animator;
using ::we::editor::toolbar::ToolButton;
using ::we::editor::toolbar::ToolButtonStyle;
using ::we::editor::menus::MenuBar;
using ::we::runtime::kindui::VerticalAlignment;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;

namespace {
    class FixedGap : public Widget {
    public:
        explicit FixedGap(float width) : m_Width(width) {}
        Size Measure(const Size& availableSize) override {
            (void)availableSize;
            m_DesiredSize = Size{ m_Width, 1.0f };
            return m_DesiredSize;
        }
        void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }
        void Paint(PaintContext& context) override { (void)context; }
    private:
        float m_Width;
    };

    class LogoSlotWidget : public Widget {
    public:
        static float SlotWidth() {
            const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
            return 24.0f * uiScale;
        }
        static float SlotHeight() {
            const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
            return we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight) * uiScale;
        }

        explicit LogoSlotWidget(we::rhi::RHIDescriptorSetHandle logoSet) : m_LogoSet(logoSet) {}

        Size Measure(const Size& availableSize) override {
            (void)availableSize;
            m_DesiredSize = Size{ SlotWidth(), SlotHeight() };
            return m_DesiredSize;
        }
        void Arrange(const Rect& allottedRect) override {
            m_Geometry = allottedRect;
        }
        void Paint(PaintContext& context) override {
            const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
            const float cx = m_Geometry.x + m_Geometry.width  * 0.5f;
            const float cy = m_Geometry.y + m_Geometry.height * 0.5f;
            const float logoSize = 24.0f * uiScale;
            const float half = logoSize * 0.5f;
            const auto snap = [](float v) { return std::floor(v + 0.5f); };
            Rect logoRect{ snap(cx - half), snap(cy - half), logoSize, logoSize };

            if (m_LogoSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
                context.DrawTexture(logoRect, m_LogoSet, ThemeColor(ColorToken::TextPrimary));
            } else {
                IconPainter::Draw(context, WindIcons::Logo24, logoRect, 24);
            }
        }
    private:
        we::rhi::RHIDescriptorSetHandle m_LogoSet;
    };

    float WindowPadLeft() { return 8.0f; }
    float LogoToMenuGap() { return 6.0f; }

    class CenteredProjectTitleWidget : public Widget {
    public:
        CenteredProjectTitleWidget() {}

        static std::string GetHeaderTitleText() {
            std::string engineVer = "0.1.0";
            if (we::projects::EngineContext::Get().IsInitialized() && !we::projects::EngineContext::Get().EngineVersion().empty()) {
                engineVer = we::projects::EngineContext::Get().EngineVersion();
            }

            std::string projName = "No Project";
            if (we::projects::ProjectContext::Get().IsLoaded()) {
                const auto& desc = we::projects::ProjectContext::Get().Descriptor();
                projName = !desc.displayName.empty() ? desc.displayName : desc.projectName;
                if (projName.empty() && !we::projects::ProjectContext::Get().WeprojPath().empty()) {
                    projName = we::projects::ProjectContext::Get().WeprojPath().stem().string();
                }
            }

            return "Windeffects (v" + engineVer + ") - " + projName;
        }

        Size Measure(const Size& availableSize) override {
            (void)availableSize;
            const std::string titleText = GetHeaderTitleText();
            const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeMenu);
            float textW = !titleText.empty() ? static_cast<float>(titleText.size()) * textSize * 0.55f : 0.0f;
            float height = we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight);
            m_DesiredSize = Size{ textW, height };
            return m_DesiredSize;
        }

        void Arrange(const Rect& allottedRect) override {
            m_Geometry = allottedRect;
        }

        void Paint(PaintContext& context) override {
            const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
            const std::string titleText = GetHeaderTitleText();

            const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
            const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeMenu) * uiScale;

            context.DrawText(titleText,
                Point{ m_Geometry.x, centerY - textSize * 0.5f },
                ThemeColor(ColorToken::TextSecondary), textSize);
        }

        bool ShowsPointerCursor(const Point&) const override { return false; }
    };

}

TitleBar::TitleBar(we::platform::WindowId window, const std::string& title, we::rhi::RHIDescriptorSetHandle logoSet,
    std::shared_ptr<::we::editor::menus::MenuBar> menuBar)
    : m_Window(window), m_Title(title), m_LogoSet(logoSet), m_MenuBar(menuBar)
{
    Padding(Margin{ 0.0f, 0.0f, 0.0f, 0.0f });
    Gap(0.0f);
}

TitleBar::~TitleBar() = default;

void TitleBar::Construct() {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_LeftContainer = std::make_shared<Row>();
    m_LeftContainer->Gap(0.0f);

    m_LogoWidget = std::make_shared<LogoSlotWidget>(m_LogoSet);
    m_LeftContainer->AddChild(m_LogoWidget);
    m_LeftContainer->AddChild(std::make_shared<FixedGap>(LogoToMenuGap() * uiScale));

    if (m_MenuBar) {
        m_MenuBar->SetHeight(we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight) * uiScale);
        m_LeftContainer->AddChild(m_MenuBar);
    }

    m_CenterContainer = std::make_shared<Row>();
    m_CenterContainer->Gap(0.0f);
    m_CenterContainer->Align(we::runtime::kindui::AlignItems::Center);
    if (!m_Title.empty()) {
        auto titleLabel = std::make_shared<::we::runtime::kindui::Label>(m_Title,
            we::runtime::kindui::TypographyToken::Caption);
        m_CenterContainer->AddChild(titleLabel);
    } else {
        auto projectWidget = std::make_shared<CenteredProjectTitleWidget>();
        m_CenterContainer->AddChild(projectWidget);
    }

    m_RightContainer = std::make_shared<Row>();
    m_RightContainer->Gap(0.0f);

    auto bookBtn = std::make_shared<ToolButton>(WindIcons::Book16, "", []() {}, "Documentation");
    auto cloudBtn = std::make_shared<ToolButton>(WindIcons::Cloud16, "", []() {}, "Cloud Services");
    auto notifBtn = std::make_shared<ToolButton>(WindIcons::Notifications16, "", []() {}, "Notifications");

    bookBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    cloudBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);
    notifBtn->SetButtonStyle(ToolButtonStyle::TitleBarTool);

    auto minimizeBtn = std::make_shared<ToolButton>(WindIcons::Minus16, "", [this]() {
        we::runtime::core::LoopExecutionTrace::Event(
            "TitleBar.MinimizeClick", "ShowWindow(SW_MINIMIZE) — WM_SIZE arrives via nested WndProc");
        if (m_Window != we::platform::WindowId::Invalid) {
            we::platform::Platform::Get().MinimizeWindow(m_Window);
        }
    });
    auto maximizeBtn = std::make_shared<ToolButton>(WindIcons::Square16, "", [this]() {
        if (m_Window != we::platform::WindowId::Invalid) {
            auto& platform = we::platform::Platform::Get();
            if (platform.IsWindowMaximized(m_Window)) {
                platform.RestoreWindow(m_Window);
            } else {
                platform.MaximizeWindow(m_Window);
            }
            UpdateMaximizeIcon();
        }
    });
    auto closeBtn = std::make_shared<ToolButton>(WindIcons::X16, "", [this]() {
        we::runtime::core::LoopExecutionTrace::Event(
            "TitleBar.CloseClick", "PushEvent(WindowCloseEvent) → pending until next PollEvents flush");
        if (m_Window != we::platform::WindowId::Invalid) {
            we::platform::Platform::Get().PushEvent(we::platform::WindowCloseEvent{m_Window});
        }
    });

    minimizeBtn->SetButtonStyle(ToolButtonStyle::WindowControl);
    maximizeBtn->SetButtonStyle(ToolButtonStyle::WindowControl);
    closeBtn->SetButtonStyle(ToolButtonStyle::WindowClose);

    minimizeBtn->SetVerticalAlignment(VerticalAlignment::Fill);
    maximizeBtn->SetVerticalAlignment(VerticalAlignment::Fill);
    closeBtn->SetVerticalAlignment(VerticalAlignment::Fill);

    m_MinimizeWidget = minimizeBtn;
    m_MaximizeWidget = maximizeBtn;
    m_CloseWidget = closeBtn;

    m_RightContainer->AddChild(bookBtn);
    m_RightContainer->AddChild(std::make_shared<FixedGap>(4.0f * uiScale));
    m_RightContainer->AddChild(cloudBtn);
    m_RightContainer->AddChild(std::make_shared<FixedGap>(4.0f * uiScale));
    m_RightContainer->AddChild(notifBtn);
    m_RightContainer->AddChild(std::make_shared<FixedGap>(14.0f * uiScale));
    m_RightContainer->AddChild(m_MinimizeWidget);
    m_RightContainer->AddChild(m_MaximizeWidget);
    m_RightContainer->AddChild(m_CloseWidget);

    UpdateMaximizeIcon();

    AddChild(m_LeftContainer);
    AddChild(m_CenterContainer);
    AddChild(m_RightContainer);

    m_InteractableWidgets.push_back(bookBtn);
    m_InteractableWidgets.push_back(cloudBtn);
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

    m_DesiredSize = Size{ availableSize.width, we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight) *
        uiScale };
    return m_DesiredSize;
}

void TitleBar::Arrange(const Rect& allottedRect) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_Geometry = allottedRect;

    if (m_RightContainer) {
        Size rightSize = m_RightContainer->GetDesiredSize();
        m_RightContainer->Arrange(Rect{
            allottedRect.x + allottedRect.width - rightSize.width,
            allottedRect.y,
            rightSize.width,
            allottedRect.height
        });
    }

    if (m_CenterContainer) {
        Size centerSize = m_CenterContainer->GetDesiredSize();
        float centerX = allottedRect.x + (allottedRect.width - centerSize.width) * 0.5f;
        m_CenterContainer->Arrange(Rect{ centerX, allottedRect.y, centerSize.width, allottedRect.height });
    }

    if (m_LeftContainer) {
        Size leftSize = m_LeftContainer->GetDesiredSize();
        m_LeftContainer->Arrange(Rect{
            allottedRect.x + WindowPadLeft() * uiScale,
            allottedRect.y,
            leftSize.width,
            allottedRect.height
        });
    }
}

void TitleBar::Paint(PaintContext& context) {
    context.PushSurfaceOwner("TitleBar", we::runtime::kindui::SurfaceRole::Window);
    context.DrawSurface(m_Geometry, we::runtime::kindui::SurfaceRole::Window, 0.0f, "TitleBar");
    Row::Paint(context);
    context.PopSurfaceOwner();
}

void TitleBar::OnMouseDown(const MouseEvent& event) {
    Row::OnMouseDown(event);
}

void TitleBar::OnMouseMove(const MouseEvent& event) {
    Row::OnMouseMove(event);
}

void TitleBar::UpdateMaximizeIcon() {
    if (m_Window == we::platform::WindowId::Invalid || !m_MaximizeWidget) return;

    auto toolBtn = std::static_pointer_cast<ToolButton>(m_MaximizeWidget);
    if (we::platform::Platform::Get().IsWindowMaximized(m_Window)) {
        toolBtn->SetIcon(WindIcons::Copy16);
    } else {
        toolBtn->SetIcon(WindIcons::Square16);
    }
}

we::platform::WindowHitTestResult TitleBar::HitTest(we::platform::Int2 point) {
    Point p{ static_cast<float>(point.x), static_cast<float>(point.y) };

    // Use live widget tree hit testing so interactive controls (menu bar, tools,
    // window controls) return Client, while container gaps remain Draggable.
    if (auto hit = HitTestPoint(p, nullptr)) {
        Widget* raw = hit.get();
        if (raw != static_cast<Widget*>(this) &&
            raw != static_cast<Widget*>(m_LeftContainer.get()) &&
            raw != static_cast<Widget*>(m_RightContainer.get()) &&
            raw != static_cast<Widget*>(m_CenterContainer.get())) {
            return we::platform::WindowHitTestResult::Client;
        }
    }

    for (const auto& w : m_InteractableWidgets) {
        if (!w) {
            continue;
        }
        const Rect& g = w->GetGeometry();
        if (g.width > 0.0f && g.height > 0.0f && g.Contains(p)) {
            return we::platform::WindowHitTestResult::Client;
        }
    }

    if (m_Geometry.Contains(p)) {
        return we::platform::WindowHitTestResult::Draggable;
    }

    return we::platform::WindowHitTestResult::Client;
}

} // namespace we::editor::shell

 
