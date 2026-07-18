#include "UI/Controls/LauncherControls.h"
#include "LauncherControlsHelpers.h"
#include "UI/Shell/LauncherHelpers.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "Platform/Events.h"
#include "Platform/PlatformSDK.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include <algorithm>
#include <cmath>
#include <memory>
using namespace we::runtime::kindui;
namespace we::programs::welauncher {
using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;
using namespace launcher_controls_detail;
FixedGap::FixedGap(float width, float height)
    : m_Width(width)
    , m_Height(height) {
}

Size FixedGap::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = Size{ m_Width, m_Height };
    return m_DesiredSize;
}

void FixedGap::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void FixedGap::Paint(PaintContext& context) {
    (void)context;
}

// Ã¢â€â‚¬Ã¢â€â‚¬ ThinDivider Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

Size ThinDivider::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 100.0f * s,
        1.0f * s
    };
    return m_DesiredSize;
}

void ThinDivider::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ThinDivider::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, LColor(ColorToken::Separator));
}
Size ThinVerticalDivider::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        1.0f * s,
        availableSize.height > 0.0f ? availableSize.height : 100.0f * s
    };
    return m_DesiredSize;
}

void ThinVerticalDivider::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ThinVerticalDivider::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, LColor(ColorToken::Separator));
}


// Ã¢â€â‚¬Ã¢â€â‚¬ LauncherTitleBar Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

LauncherTitleBar::LauncherTitleBar(we::platform::WindowId window, std::string title)
    : m_Window(window)
    , m_Title(std::move(title)) {
}

void LauncherTitleBar::SetLogoTexture(we::rhi::RHIDescriptorSetHandle logoSet) {
    m_LogoSet = logoSet;
    InvalidateUI();
}

void LauncherTitleBar::UpdateMaximizeIcon() {
    if (m_Window == we::platform::WindowId::Invalid) {
        return;
    }
    m_IsMaximized = we::platform::Platform::Get().IsWindowMaximized(m_Window);
    InvalidateUI();
}

Size LauncherTitleBar::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, kLauncherTitleBarH * LScale() };
    return m_DesiredSize;
}

void LauncherTitleBar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    LayoutRects();
    UpdateMaximizeIcon();
}

void LauncherTitleBar::LayoutRects() {
    const float s = LScale();
    const float h = m_Geometry.height;
    const float padL = kLauncherHeaderPadL * s;
    const float controlW = kLauncherWindowControlW * s;
    const float gap = LMetric(MetricToken::Space2) * s;
    const float groupGap = LMetric(MetricToken::ButtonGroupSpacing) * s;

    m_CloseRect = Rect{ m_Geometry.x + m_Geometry.width - controlW, m_Geometry.y, controlW, h };
    m_MaxRect = Rect{ m_CloseRect.x - controlW, m_Geometry.y, controlW, h };
    m_MinRect = Rect{ m_MaxRect.x - controlW, m_Geometry.y, controlW, h };

    const float actionH = LMetric(MetricToken::HeaderControlHeight) * s;
    const float actionW = actionH;
    const float actionY = m_Geometry.y + (h - actionH) * 0.5f;
    float right = m_MinRect.x - groupGap;
    m_SettingsRect = Rect{ right - actionW, actionY, actionW, actionH };
    right = m_SettingsRect.x - gap;
    m_HelpRect = Rect{ right - actionW, actionY, actionW, actionH };

    const float brandW = 220.0f * s;
    m_BrandRect = Rect{ m_Geometry.x + padL, m_Geometry.y, brandW, h };
}

LauncherTitleBar::HitZone LauncherTitleBar::HitTest(const Point& p) const {
    if (m_CloseRect.Contains(p)) return HitZone::Close;
    if (m_MaxRect.Contains(p)) return HitZone::Maximize;
    if (m_MinRect.Contains(p)) return HitZone::Minimize;
    if (m_SettingsRect.Contains(p)) return HitZone::Settings;
    if (m_HelpRect.Contains(p)) return HitZone::Help;
    if (m_Geometry.Contains(p)) return HitZone::Drag;
    return HitZone::None;
}

void LauncherTitleBar::PaintIconButton(
    PaintContext& context,
    const Rect& r,
    const char* icon,
    float hover,
    bool danger) const {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;

    Color bg = Color::Transparent();
    if (hover > 0.01f) {
        bg = Color::Lerp(
            bg,
            danger ? LColor(ColorToken::CloseButtonHover) : LColor(ColorToken::HoverBackground),
            hover);
    }
    if (bg.a > 0.01f) {
        context.DrawRoundedRect(r, bg, radius);
    }

    const float glyph = kLauncherIconPx * s;
    IconPainter::DrawIcon(
        context,
        icon,
        Rect{ r.x + (r.width - glyph) * 0.5f, r.y + (r.height - glyph) * 0.5f, glyph, glyph },
        LColor(ColorToken::IconSecondary));
}

void LauncherTitleBar::Paint(PaintContext& context) {
    const float s = LScale();
    context.DrawRect(m_Geometry, LColor(ColorToken::HeaderBackground));
    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f * s, m_Geometry.width, 1.0f * s },
        LColor(ColorToken::Separator));

    const float logoSize = kLauncherLogoDisplaySize * s;
    const float logoX = m_BrandRect.x;
    const float logoY = m_Geometry.y + (m_Geometry.height - logoSize) * 0.5f;
    Rect logoRect{ logoX, logoY, logoSize, logoSize };
    IconPainter::DrawIcon(context, Icons::WindLogoName, logoRect, Color::White());

    const float titleSize = LMetric(MetricToken::TextSizeToolbar) * s;
    context.DrawText(
        m_Title,
        Point{ logoRect.x + logoSize + LMetric(MetricToken::Space2) * s, m_Geometry.y + (m_Geometry.height - titleSize) * 0.5f },
        LColor(ColorToken::TextWindowLabel),
        titleSize,
        true);

    PaintIconButton(context, m_HelpRect, Icons::InfoName, m_HoverHelp);
    PaintIconButton(context, m_SettingsRect, Icons::SettingsName, m_HoverSettings);

    const char* maxIcon = m_IsMaximized ? Icons::RestoreName : Icons::MaximizeName;
    const char* icons[3] = { Icons::MinimizeName, maxIcon, Icons::XName };
    const Rect controls[3] = { m_MinRect, m_MaxRect, m_CloseRect };
    const float hovers[3] = { m_HoverMin, m_HoverMax, m_HoverClose };
    for (int i = 0; i < 3; ++i) {
        const Rect& r = controls[i];
        Color bg = Color::Transparent();
        if (i == 2 && hovers[i] > 0.01f) {
            bg = Color::Lerp(bg, LColor(ColorToken::CloseButtonHover), hovers[i]);
        } else if (hovers[i] > 0.01f) {
            bg = Color::Lerp(bg, LColor(ColorToken::HoverBackground), hovers[i]);
        }
        if (bg.a > 0.01f) {
            context.DrawRect(r, bg);
        }
        const float glyph = kLauncherIconPx * s;
        IconPainter::DrawIcon(
            context,
            icons[i],
            Rect{ r.x + (r.width - glyph) * 0.5f, r.y + (r.height - glyph) * 0.5f, glyph, glyph },
            LColor(ColorToken::IconSecondary));
    }
}

void LauncherTitleBar::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    m_PressedZone = HitTest(event.position);
}

void LauncherTitleBar::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const HitZone hit = HitTest(event.position);
    if (hit != m_PressedZone) {
        m_PressedZone = HitZone::None;
        return;
    }

    auto& platform = we::platform::Platform::Get();
    switch (hit) {
    case HitZone::Minimize:
        platform.MinimizeWindow(m_Window);
        break;
    case HitZone::Maximize:
        if (platform.IsWindowMaximized(m_Window)) {
            platform.RestoreWindow(m_Window);
        } else {
            platform.MaximizeWindow(m_Window);
        }
        UpdateMaximizeIcon();
        break;
    case HitZone::Close:
        platform.PushEvent(we::platform::WindowCloseEvent{ m_Window });
        break;
    case HitZone::Help:
        if (m_OnHelp) {
            m_OnHelp();
        }
        break;
    case HitZone::Settings:
        if (m_OnSettings) {
            m_OnSettings();
        }
        break;
    default:
        break;
    }
    m_PressedZone = HitZone::None;
}

void LauncherTitleBar::OnMouseMove(const MouseEvent& event) {
    m_HoverZone = HitTest(event.position);
    m_Hovered = m_HoverZone != HitZone::None && m_HoverZone != HitZone::Drag;
}

bool LauncherTitleBar::ShowsPointerCursor(const Point& position) const {
    const HitZone z = HitTest(position);
    return z != HitZone::None && z != HitZone::Drag;
}

we::platform::WindowHitTestResult LauncherTitleBar::WindowHitTest(we::platform::Int2 point) const {
    const Point p{ static_cast<float>(point.x), static_cast<float>(point.y) };
    const HitZone zone = HitTest(p);
    switch (zone) {
    case HitZone::Minimize:
    case HitZone::Maximize:
    case HitZone::Close:
    case HitZone::Help:
    case HitZone::Settings:
        return we::platform::WindowHitTestResult::Client;
    case HitZone::Drag:
        return we::platform::WindowHitTestResult::Draggable;
    default:
        return we::platform::WindowHitTestResult::Client;
    }
}

void LauncherTitleBar::Tick(float deltaTime) {
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    m_HoverMin = Animator::Damp(m_HoverMin, m_HoverZone == HitZone::Minimize ? 1.0f : 0.0f, damp);
    m_HoverMax = Animator::Damp(m_HoverMax, m_HoverZone == HitZone::Maximize ? 1.0f : 0.0f, damp);
    m_HoverClose = Animator::Damp(m_HoverClose, m_HoverZone == HitZone::Close ? 1.0f : 0.0f, damp);
    m_HoverHelp = Animator::Damp(m_HoverHelp, m_HoverZone == HitZone::Help ? 1.0f : 0.0f, damp);
    m_HoverSettings = Animator::Damp(m_HoverSettings, m_HoverZone == HitZone::Settings ? 1.0f : 0.0f, damp);
    Widget::Tick(deltaTime);
}

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ NavSidebar ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬

} // namespace we::programs::welauncher
