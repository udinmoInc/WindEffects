#include "Widgets/TitleBar.h"
#include "Widgets/MenuBar.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeColors.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Widgets/Label.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "Widgets/ToolButton.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "Platform/Platform.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::shell {
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;
using ::we::runtime::kindui::IconPainter;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;

using ::we::editor::panels::Panel;

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
        static constexpr float kSlotSize = 28.0f;

        explicit LogoSlotWidget(we::rhi::RHIDescriptorSetHandle logoSet) : m_LogoSet(logoSet) {}

        Size Measure(const Size& availableSize) override {
            (void)availableSize;
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
            const float half = kTitleBarLogoDisplaySize * 0.5f;
            const auto snap = [](float v) { return std::floor(v + 0.5f); };
            Rect logoRect{
                snap(cx - half),
                snap(cy - half),
                kTitleBarLogoDisplaySize,
                kTitleBarLogoDisplaySize
            };

            if ((m_LogoSet != we::rhi::RHIDescriptorSetHandle::Invalid)) {
                context.DrawTexture(logoRect, m_LogoSet, ThemeColor(ColorToken::IconPrimary));
            } else {
                IconPainter::DrawIcon(context, Icons::CameraName, logoRect, ThemeColor(ColorToken::IconPrimary));
            }
        }
    private:
        we::rhi::RHIDescriptorSetHandle m_LogoSet;
    };

    class ProjectSelectorWidget : public Widget {
    public:
        static constexpr float kHeight   = kHeaderControlHeight;
        static constexpr float kPadH     = 8.0f;
        static constexpr float kIconSize = 16.0f;
        static constexpr float kIconGap  = 8.0f;
        static constexpr float kChevGap  = 8.0f;
        static constexpr float kTextSize = 12.0f;
        static constexpr const char* kProjectName = "MyProject";

        ProjectSelectorWidget() {}
        Size Measure(const Size& availableSize) override {
            (void)availableSize;
            float textW = kProjectName[0] ? static_cast<float>(strlen(kProjectName)) * 6.8f : 0.0f;
            float width = kPadH + kIconSize + kIconGap + textW + kChevGap + IconMetrics::CompactDisplayPx() + kPadH;
            m_DesiredSize = Size{ width, kHeight };
            return m_DesiredSize;
        }
        void Arrange(const Rect& allottedRect) override {
            m_Geometry = allottedRect;
            if (allottedRect.height > m_DesiredSize.height) {
                m_Geometry.y += (allottedRect.height - m_DesiredSize.height) * 0.5f;
                m_Geometry.height = m_DesiredSize.height;
            }
        }
        void Paint(PaintContext& context) override {
            const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
            m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ThemeMetric(MetricToken::HoverAnimationDamping));

            const float radius = ThemeMetric(MetricToken::CornerRadiusMedium) * uiScale;
            if (m_HoverAnim > 0.01f) {
                Color hoverBg = Color::Lerp(
                    ThemeColor(ColorToken::ButtonPrimaryBackground),
                    ThemeColor(ColorToken::HoverBackground),
                    m_HoverAnim);
                context.DrawRoundedRect(m_Geometry, hoverBg, radius);
            } else {
                Color idleBg = ThemeColor(ColorToken::ButtonPrimaryBackground);
                idleBg.a = 0.65f;
                context.DrawRoundedRect(m_Geometry, idleBg, radius);
            }

            const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
            const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(kIconSize));
            const float textSize = kTextSize * uiScale;

            Color iconColor = ResolveIconColor(IconColorRole::Secondary, m_HoverAnim);

            IconPainter::DrawIcon(context, Icons::PackageName,
                Rect{ m_Geometry.x + kPadH * uiScale, centerY - iconSize * 0.5f, iconSize, iconSize },
                iconColor);

            const float textX = m_Geometry.x + (kPadH + iconSize + kIconGap) * uiScale;
            context.DrawText(kProjectName,
                Point{ textX, centerY - textSize * 0.5f },
                ThemeColor(ColorToken::TextPrimary), textSize);

            const float display = IconMetrics::CompactDisplayPx();
            const float chevronX = m_Geometry.x + m_Geometry.width - (kPadH + display) * uiScale;
            Rect chevronControl{ chevronX, centerY - display * 0.5f, display, display };
            IconPainter::DrawCompactIcon(context, Icons::ChevronDownName, chevronControl, iconColor);
        }
        bool ShowsPointerCursor(const Point&) const override { return true; }
    private:
        float m_HoverAnim = 0.0f;
    };

    constexpr float kWindowPadLeft = 16.0f;
    constexpr float kLogoToMenuGap = 8.0f;
}

TitleBar::TitleBar(we::platform::WindowId window, const std::string& title, we::rhi::RHIDescriptorSetHandle logoSet, std::shared_ptr<MenuBar> menuBar)
    : m_Window(window), m_Title(title), m_LogoSet(logoSet), m_MenuBar(menuBar)
{
    Padding(Margin{ 0.0f, 0.0f, 0.0f, 0.0f });
    Gap(0.0f);
}

void TitleBar::Construct() {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_LeftContainer = std::make_shared<Row>();
    m_LeftContainer->Gap(0.0f);

    m_LogoWidget = std::make_shared<LogoSlotWidget>(m_LogoSet);
    m_LeftContainer->AddChild(m_LogoWidget);
    m_LeftContainer->AddChild(std::make_shared<FixedGap>(kLogoToMenuGap * uiScale));

    if (m_MenuBar) {
        m_MenuBar->SetHeight(kTitleBarHeight * uiScale);
        m_LeftContainer->AddChild(m_MenuBar);
    }

    m_CenterContainer = std::make_shared<Row>();
    m_CenterContainer->Gap(0.0f);

    m_RightContainer = std::make_shared<Row>();
    m_RightContainer->Gap(0.0f);

    auto minimizeBtn = std::make_shared<ToolButton>(Icons::MinimizeName, "", [this]() {
        if (m_Window != we::platform::WindowId::Invalid) {
            we::platform::Platform::Get().MinimizeWindow(m_Window);
        }
    });
    auto maximizeBtn = std::make_shared<ToolButton>(Icons::MaximizeName, "", [this]() {
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
    auto closeBtn = std::make_shared<ToolButton>(Icons::XName, "", [this]() {
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

    m_RightContainer->AddChild(m_MinimizeWidget);
    m_RightContainer->AddChild(m_MaximizeWidget);
    m_RightContainer->AddChild(m_CloseWidget);

    UpdateMaximizeIcon();

    AddChild(m_LeftContainer);
    AddChild(m_RightContainer);

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
            allottedRect.x + kWindowPadLeft * uiScale,
            allottedRect.y,
            leftSize.width,
            allottedRect.height
        });
    }
}

void TitleBar::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());

    context.DrawRect(m_Geometry, ThemeColor(ColorToken::WindowBackground));

    Rect bottomEdge{
        m_Geometry.x,
        m_Geometry.y + m_Geometry.height - 1.0f * uiScale,
        m_Geometry.width,
        1.0f * uiScale
    };
    context.DrawRect(bottomEdge, ThemeColor(ColorToken::BorderDark));

    Row::Paint(context);
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
        toolBtn->SetIcon(Icons::RestoreName);
    } else {
        toolBtn->SetIcon(Icons::MaximizeName);
    }
}

we::platform::WindowHitTestResult TitleBar::HitTest(we::platform::Int2 point) {
    Point p{ static_cast<float>(point.x), static_cast<float>(point.y) };

    for (const auto& w : m_InteractableWidgets) {
        if (p.x >= w->GetGeometry().x && p.x <= w->GetGeometry().x + w->GetGeometry().width &&
            p.y >= w->GetGeometry().y && p.y <= w->GetGeometry().y + w->GetGeometry().height) {
            return we::platform::WindowHitTestResult::Client;
        }
    }

    if (p.x >= m_Geometry.x && p.x <= m_Geometry.x + m_Geometry.width &&
        p.y >= m_Geometry.y && p.y <= m_Geometry.y + m_Geometry.height) {
        return we::platform::WindowHitTestResult::Draggable;
    }

    return we::platform::WindowHitTestResult::Client;
}

} // namespace we::editor::shell