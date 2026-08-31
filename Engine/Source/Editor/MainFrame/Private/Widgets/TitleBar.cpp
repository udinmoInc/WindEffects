#include "Widgets/TitleBar.h"
#include "Widgets/MenuBar.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
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
        static float SlotSize() {
            return we::runtime::kindui::ResolveMetric(MetricToken::PanelToolbarHeight);
        }

        explicit LogoSlotWidget(we::rhi::RHIDescriptorSetHandle logoSet) : m_LogoSet(logoSet) {}

        Size Measure(const Size& availableSize) override {
            (void)availableSize;
            const float slot = SlotSize();
            m_DesiredSize = Size{ slot, slot };
            return m_DesiredSize;
        }
        void Arrange(const Rect& allottedRect) override {
            m_Geometry = allottedRect;
            const float slot = SlotSize();
            if (allottedRect.height > slot) {
                m_Geometry.y += (allottedRect.height - slot) * 0.5f;
                m_Geometry.height = slot;
            }
        }
        void Paint(PaintContext& context) override {
            const float cx = m_Geometry.x + m_Geometry.width  * 0.5f;
            const float cy = m_Geometry.y + m_Geometry.height * 0.5f;
            const float logoSize = we::runtime::kindui::ResolveMetric(MetricToken::IconSizePrimary)
                + we::runtime::kindui::ResolveMetric(MetricToken::Space1) * 0.5f;
            const float half = logoSize * 0.5f;
            const auto snap = [](float v) { return std::floor(v + 0.5f); };
            Rect logoRect{
                snap(cx - half),
                snap(cy - half),
                logoSize,
                logoSize
            };

            if ((m_LogoSet != we::rhi::RHIDescriptorSetHandle::Invalid)) {
                context.DrawTexture(logoRect, m_LogoSet, ThemeColor(ColorToken::TextPrimary));
            } else {
                IconPainter::Draw(context, kWindIconNone, logoRect);
            }
        }
    private:
        we::rhi::RHIDescriptorSetHandle m_LogoSet;
    };

    class ProjectSelectorWidget : public Widget {
    public:
        static constexpr const char* kProjectName = "MyProject";

        static float ControlHeight() {
            return we::runtime::kindui::ResolveMetric(MetricToken::HeaderControlHeight);
        }

        ProjectSelectorWidget() {}
        Size Measure(const Size& availableSize) override {
            (void)availableSize;
            const float padH = we::runtime::kindui::ResolveMetric(MetricToken::Space2);
            const float iconSize = we::runtime::kindui::ResolveMetric(MetricToken::IconSizePrimary);
            const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeMenu);
            float textW = kProjectName[0] ? static_cast<float>(strlen(kProjectName)) * textSize * 0.55f : 0.0f;
            float width = padH + iconSize + padH + textW + padH + static_cast<float>(16u) + padH;
            m_DesiredSize = Size{ width, ControlHeight() };
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
                context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::ButtonPrimaryBackground), radius);
            }

            const float padH = we::runtime::kindui::ResolveMetric(MetricToken::Space2);
            const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
            const float iconSize = 16.0f;
            const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeMenu) * uiScale;

            Color iconColor = ResolveIconColor(IconColorRole::Secondary, m_HoverAnim);

            IconPainter::Draw(context, kWindIconNone, Rect{ m_Geometry.x + padH * uiScale, centerY - iconSize * 0.5f, iconSize, iconSize });

            const float textX = m_Geometry.x + (padH + iconSize + padH) * uiScale;
            context.DrawText(kProjectName,
                Point{ textX, centerY - textSize * 0.5f },
                ThemeColor(ColorToken::TextPrimary), textSize);

            const float tier = static_cast<float>(16u);
            const float chevronX = m_Geometry.x + m_Geometry.width - (padH + tier) * uiScale;
            IconPainter::Draw(
                context, WindIcons::ChevronDown16, IconMetrics::CompactGlyphBand(m_Geometry, chevronX));
        }
        bool ShowsPointerCursor(const Point&) const override { return true; }
    private:
        float m_HoverAnim = 0.0f;
    };

    float WindowPadLeft() { return we::runtime::kindui::ResolveMetric(MetricToken::Space4); }
    float LogoToMenuGap() { return we::runtime::kindui::ResolveMetric(MetricToken::Space2); }
}

TitleBar::TitleBar(we::platform::WindowId window, const std::string& title, we::rhi::RHIDescriptorSetHandle logoSet, std::shared_ptr<::we::editor::menus::MenuBar> menuBar)
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
    m_LeftContainer->AddChild(std::make_shared<FixedGap>(LogoToMenuGap() * uiScale));

    if (m_MenuBar) {
        m_MenuBar->SetHeight(we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight) * uiScale);
        m_LeftContainer->AddChild(m_MenuBar);
    }

    m_CenterContainer = std::make_shared<Row>();
    m_CenterContainer->Gap(0.0f);

    m_RightContainer = std::make_shared<Row>();
    m_RightContainer->Gap(0.0f);

    auto minimizeBtn = std::make_shared<ToolButton>(kWindIconNone, "", [this]() {
        if (m_Window != we::platform::WindowId::Invalid) {
            we::platform::Platform::Get().MinimizeWindow(m_Window);
        }
    });
    auto maximizeBtn = std::make_shared<ToolButton>(kWindIconNone, "", [this]() {
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
    auto closeBtn = std::make_shared<ToolButton>(WindIcons::Close16, "", [this]() {
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

    m_DesiredSize = Size{ availableSize.width, we::runtime::kindui::ResolveMetric(MetricToken::TitleBarHeight) * uiScale };
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
        toolBtn->SetIcon(kWindIconNone);
    } else {
        toolBtn->SetIcon(kWindIconNone);
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
