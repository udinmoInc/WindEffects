#include "UI/Widgets/LauncherControls.h"

#include "UI/LauncherHelpers.h"

#include "Core/Animator.h"
#include "Core/ControlChrome.h"
#include "Core/DPIContext.h"
#include "Core/EventSystem.h"
#include "Core/Icon.h"
#include "Core/PaintContext.h"
#include "Rendering/IconMetrics.h"
#include "Platform/Events.h"
#include "Platform/PlatformSDK.h"
#include "WindEffects/Editor/UI/Theming/ThemeManager.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <algorithm>
#include <cmath>
#include <memory>

using namespace WindEffects::Editor::UI;

namespace we::programs::welauncher {
namespace {

constexpr float kSegmentH = 28.0f;

bool AppendUtf8(std::string& out, char32_t cp) {
    if (cp < 32 && cp != '\t') {
        return false;
    }
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        return false;
    }
    return true;
}

float ApproxTextWidth(const std::string& text, float textSize) {
    return static_cast<float>(text.size()) * textSize * 0.55f;
}

void EraseLastUtf8Codepoint(std::string& text) {
    if (text.empty()) {
        return;
    }
    while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80) {
        text.pop_back();
    }
    if (!text.empty()) {
        text.pop_back();
    }
}

} // namespace

// Ã¢â€â‚¬Ã¢â€â‚¬ FixedGap Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

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

// â”€â”€ ThinDivider â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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
    context.DrawRect(m_Geometry, LColor(ThemeToken::Separator));
}

// â”€â”€ LauncherTitleBar â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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
    const float gap = LMetric(ThemeToken::Space2) * s;
    const float groupGap = LMetric(ThemeToken::ButtonGroupSpacing) * s;

    m_CloseRect = Rect{ m_Geometry.x + m_Geometry.width - controlW, m_Geometry.y, controlW, h };
    m_MaxRect = Rect{ m_CloseRect.x - controlW, m_Geometry.y, controlW, h };
    m_MinRect = Rect{ m_MaxRect.x - controlW, m_Geometry.y, controlW, h };

    const float actionH = LMetric(ThemeToken::HeaderControlHeight) * s;
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
    const float radius = LMetric(ThemeToken::CornerRadiusSmall) * s;

    Color bg = Color::Transparent();
    if (hover > 0.01f) {
        bg = Color::Lerp(
            bg,
            danger ? LColor(ThemeToken::CloseButtonHover) : LColor(ThemeToken::HoverBackground),
            hover);
    }
    if (bg.a > 0.01f) {
        context.DrawRoundedRect(r, bg, radius);
    }

    const float glyph = 14.0f * s;
    IconPainter::DrawIcon(
        context,
        icon,
        Rect{ r.x + (r.width - glyph) * 0.5f, r.y + (r.height - glyph) * 0.5f, glyph, glyph },
        LColor(ThemeToken::IconSecondary));
}

void LauncherTitleBar::Paint(PaintContext& context) {
    const float s = LScale();
    context.DrawRect(m_Geometry, LColor(ThemeToken::HeaderBackground));
    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f * s, m_Geometry.width, 1.0f * s },
        LColor(ThemeToken::Separator));

    const float logoSize = kLauncherLogoDisplaySize * s;
    const float logoX = m_BrandRect.x;
    const float logoY = m_Geometry.y + (m_Geometry.height - logoSize) * 0.5f;
    Rect logoRect{ logoX, logoY, logoSize, logoSize };
    if (m_LogoSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        context.DrawTexture(logoRect, m_LogoSet, LColor(ThemeToken::IconPrimary));
    } else {
        IconPainter::DrawIcon(context, Icons::PackageName, logoRect, LColor(ThemeToken::IconPrimary));
    }

    const float titleSize = LMetric(ThemeToken::TextSizeToolbar) * s;
    context.DrawText(
        m_Title,
        Point{ logoRect.x + logoSize + LMetric(ThemeToken::Space2) * s, m_Geometry.y + (m_Geometry.height - titleSize) * 0.5f },
        LColor(ThemeToken::TextWindowLabel),
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
            bg = Color::Lerp(bg, LColor(ThemeToken::CloseButtonHover), hovers[i]);
        } else if (hovers[i] > 0.01f) {
            bg = Color::Lerp(bg, LColor(ThemeToken::HoverBackground), hovers[i]);
        }
        if (bg.a > 0.01f) {
            context.DrawRect(r, bg);
        }
        const float glyph = 14.0f * s;
        IconPainter::DrawIcon(
            context,
            icons[i],
            Rect{ r.x + (r.width - glyph) * 0.5f, r.y + (r.height - glyph) * 0.5f, glyph, glyph },
            LColor(ThemeToken::IconSecondary));
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
    const float damp = LMetric(ThemeToken::HoverAnimationDamping);
    m_HoverMin = Animator::Damp(m_HoverMin, m_HoverZone == HitZone::Minimize ? 1.0f : 0.0f, damp);
    m_HoverMax = Animator::Damp(m_HoverMax, m_HoverZone == HitZone::Maximize ? 1.0f : 0.0f, damp);
    m_HoverClose = Animator::Damp(m_HoverClose, m_HoverZone == HitZone::Close ? 1.0f : 0.0f, damp);
    m_HoverHelp = Animator::Damp(m_HoverHelp, m_HoverZone == HitZone::Help ? 1.0f : 0.0f, damp);
    m_HoverSettings = Animator::Damp(m_HoverSettings, m_HoverZone == HitZone::Settings ? 1.0f : 0.0f, damp);
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ NavSidebar Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

NavSidebar::NavSidebar() {
    m_Items = {
        { LauncherPage::Projects, "Projects", Icons::OpenFolderName },
        { LauncherPage::Templates, "Templates", Icons::LayersName },
        { LauncherPage::Library, "Library", Icons::Cube3DName },
        { LauncherPage::Engine, "Engine", Icons::BuildName },
        { LauncherPage::Settings, "Settings", Icons::SettingsName },
    };
    m_WidthAnim = kLauncherNavWidth;
}

float NavSidebar::PreferredWidth() const {
    return m_WidthAnim * LScale();
}

void NavSidebar::SetActivePage(LauncherPage page) {
    m_Active = page;
    InvalidateUI();
}

void NavSidebar::SetCollapsed(bool collapsed) {
    if (m_Collapsed == collapsed) {
        return;
    }
    m_Collapsed = collapsed;
    InvalidateUI();
}

int NavSidebar::IndexOfPage(LauncherPage page) const {
    for (int i = 0; i < static_cast<int>(m_Items.size()); ++i) {
        if (m_Items[static_cast<std::size_t>(i)].page == page) {
            return i;
        }
    }
    return -1;
}

bool NavSidebar::NavigateToIndex(int index) {
    if (index < 0 || index >= static_cast<int>(m_Items.size())) {
        return false;
    }
    m_Active = m_Items[static_cast<std::size_t>(index)].page;
    if (m_OnPageChanged) {
        m_OnPageChanged(m_Active);
    }
    InvalidateUI();
    return true;
}

bool NavSidebar::NavigateByDelta(int delta) {
    const int current = IndexOfPage(m_Active);
    if (current < 0) {
        return NavigateToIndex(0);
    }
    const int next = std::clamp(current + delta, 0, static_cast<int>(m_Items.size()) - 1);
    if (next == current) {
        return false;
    }
    return NavigateToIndex(next);
}

Size NavSidebar::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ PreferredWidth(), availableSize.height };
    return m_DesiredSize;
}

void NavSidebar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

Rect NavSidebar::ItemRect(int index) const {
    const float s = LScale();
    const float padH = LMetric(ThemeToken::Space2) * s;
    const float padTop = LMetric(ThemeToken::Space4) * s;
    const float itemH = kLauncherNavItemH * s;
    const float gap = LMetric(ThemeToken::Space2) * s;
    const float top = m_Geometry.y + padTop + static_cast<float>(index) * (itemH + gap);
    return Rect{
        m_Geometry.x + padH,
        top,
        m_Geometry.width - padH * 2.0f,
        itemH
    };
}

int NavSidebar::HitItem(const Point& p) const {
    for (int i = 0; i < static_cast<int>(m_Items.size()); ++i) {
        if (ItemRect(i).Contains(p)) {
            return i;
        }
    }
    return -1;
}

void NavSidebar::Paint(PaintContext& context) {
    const float s = LScale();
    context.DrawRect(m_Geometry, LColor(ThemeToken::PanelBackground));
    // Right edge separator is drawn by ThinVerticalDivider in the shell chrome.

    for (int i = 0; i < static_cast<int>(m_Items.size()); ++i) {
        auto& item = m_Items[static_cast<std::size_t>(i)];
        const Rect r = ItemRect(i);
        const bool active = item.page == m_Active;

        ControlChrome::InteractionState state{
            item.hoverAnim,
            0.0f,
            active || item.selectAnim > 0.5f,
            false,
            false
        };
        if (active || item.selectAnim > 0.01f || item.hoverAnim > 0.01f) {
            const ResolvedStyle style = ThemeManager::Get().Resolve(
                active ? StyleRole::SidebarItemActive : StyleRole::SidebarItem);
            Color bg = Color::Transparent();
            if (item.selectAnim > 0.01f) {
                bg = Color::Lerp(bg, style.background.a > 0.01f
                    ? style.background
                    : LColor(ThemeToken::SelectedBackground), item.selectAnim);
            } else if (item.hoverAnim > 0.01f) {
                bg = Color::Lerp(bg, LColor(ThemeToken::HoverBackground), item.hoverAnim);
            }
            if (bg.a > 0.01f) {
                context.DrawRoundedRect(r, bg, style.cornerRadius);
            }
            if (item.selectAnim > 0.01f) {
                Color accent = LColor(ThemeToken::AccentPrimary);
                accent.a *= item.selectAnim;
                context.DrawRect(
                    Rect{ r.x, r.y + 6.0f * s, 3.0f * s, r.height - 12.0f * s },
                    accent);
            }
        }
        (void)state;

        const ResolvedStyle itemStyle = ThemeManager::Get().Resolve(
            active ? StyleRole::SidebarItemActive : StyleRole::SidebarItem);
        const float iconSize = itemStyle.iconSize > 0.0f
            ? itemStyle.iconSize
            : static_cast<float>(IconMetrics::NativeIconTierPx(LMetric(ThemeToken::IconSizeNavigation)));
        const float iconPad = LMetric(ThemeToken::Space2) * s;
        const float iconX = r.x + (m_Collapsed ? (r.width - iconSize) * 0.5f : iconPad);
        IconPainter::DrawIcon(
            context,
            item.icon,
            Rect{ iconX, r.y + (r.height - iconSize) * 0.5f, iconSize, iconSize },
            itemStyle.icon.a > 0.01f ? itemStyle.icon : (active
                ? LColor(ThemeToken::IconAccent)
                : LColor(ThemeToken::IconSecondary)));

        if (!m_Collapsed) {
            const float textSize = itemStyle.fontSize > 0.0f
                ? itemStyle.fontSize
                : LMetric(ThemeToken::TextSizeBody) * s;
            context.DrawText(
                item.label,
                Point{ iconX + iconSize + LMetric(ThemeToken::Space2) * s, r.y + (r.height - textSize) * 0.5f },
                itemStyle.foreground,
                textSize,
                active || itemStyle.bold);
        }
    }
}

void NavSidebar::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_PressedIndex = HitItem(event.position);
    }
}

void NavSidebar::OnMouseMove(const MouseEvent& event) {
    m_HoveredIndex = HitItem(event.position);
    m_Hovered = m_HoveredIndex >= 0;
}

void NavSidebar::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_PressedIndex >= 0) {
        const int hit = HitItem(event.position);
        if (hit == m_PressedIndex && hit >= 0) {
            NavigateToIndex(hit);
        }
    }
    m_PressedIndex = -1;
}

bool NavSidebar::ShowsPointerCursor(const Point& position) const {
    return HitItem(position) >= 0;
}

void NavSidebar::Tick(float deltaTime) {
    const float damp = LMetric(ThemeToken::HoverAnimationDamping);
    const float targetW = m_Collapsed ? kLauncherNavCollapsedWidth : kLauncherNavWidth;
    const float prevW = m_WidthAnim;
    m_WidthAnim = Animator::Damp(m_WidthAnim, targetW, damp);
    if (std::abs(m_WidthAnim - prevW) > 0.05f) {
        InvalidateUI();
    }

    for (int i = 0; i < static_cast<int>(m_Items.size()); ++i) {
        auto& item = m_Items[static_cast<std::size_t>(i)];
        const bool active = item.page == m_Active;
        const bool hovered = (i == m_HoveredIndex) && !active;
        item.hoverAnim = Animator::Damp(item.hoverAnim, hovered ? 1.0f : 0.0f, damp);
        item.selectAnim = Animator::Damp(item.selectAnim, active ? 1.0f : 0.0f, damp);
    }
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ SearchField Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

SearchField::SearchField() = default;

void SearchField::SetText(const std::string& text) {
    m_Text = text;
    if (m_OnTextChanged) {
        m_OnTextChanged(m_Text);
    }
}

void SearchField::AppendCodepoint(char32_t codepoint) {
    if (m_Text.size() >= 128) {
        return;
    }
    if (AppendUtf8(m_Text, codepoint) && m_OnTextChanged) {
        m_OnTextChanged(m_Text);
    }
}

Size SearchField::Measure(const Size& availableSize) {
    const float s = LScale();
    const float w = availableSize.width > 0.0f ? std::min(availableSize.width, 320.0f * s) : 280.0f * s;
    m_DesiredSize = Size{ w, kLauncherSearchH * s };
    return m_DesiredSize;
}

void SearchField::Arrange(const Rect& allottedRect) {
    const float h = kLauncherSearchH * LScale();
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        allottedRect.width,
        h
    };
}

Rect SearchField::ClearRect() const {
    const float s = LScale();
    const float size = 14.0f * s;
    return Rect{
        m_Geometry.x + m_Geometry.width - size - 8.0f * s,
        m_Geometry.y + (m_Geometry.height - size) * 0.5f,
        size,
        size
    };
}

void SearchField::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(ThemeToken::CornerRadiusSmall) * s;

    Color bg = LColor(ThemeToken::SearchBoxBackground);
    if (m_HoverAnim > 0.01f || m_FocusAnim > 0.01f) {
        bg = Color::Lerp(bg, LColor(ThemeToken::HoverBackground), std::max(m_HoverAnim, m_FocusAnim) * 0.35f);
    }
    context.DrawRoundedRect(m_Geometry, bg, radius);

    Color border = LColor(ThemeToken::BorderDefault);
    if (m_FocusAnim > 0.01f) {
        border = Color::Lerp(border, LColor(ThemeToken::BorderFocus), m_FocusAnim);
    }
    context.DrawRoundedRectOutline(m_Geometry, border, 1.0f, radius);

    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(LMetric(ThemeToken::IconSizeSearch)));
    const float pad = 10.0f * s;
    IconPainter::DrawIcon(
        context,
        Icons::SearchName,
        Rect{ m_Geometry.x + pad, m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f, iconSize, iconSize },
        LColor(ThemeToken::IconSecondary));

    const float textSize = LMetric(ThemeToken::TextSizeBody) * s;
    const float textX = m_Geometry.x + pad + iconSize + 8.0f * s;
    const float textY = m_Geometry.y + (m_Geometry.height - textSize) * 0.5f;
    if (m_Text.empty()) {
        context.DrawText(m_Placeholder, Point{ textX, textY }, LColor(ThemeToken::SearchPlaceholder), textSize);
    } else {
        context.DrawText(m_Text, Point{ textX, textY }, LColor(ThemeToken::TextPrimary), textSize);
        IconPainter::DrawIcon(context, Icons::XName, ClearRect(), LColor(ThemeToken::IconSecondary));
    }

    if (m_Focused && m_ShowCaret) {
        const float caretX = textX + ApproxTextWidth(m_Text, textSize);
        context.DrawRect(Rect{ caretX, textY, 1.0f * s, textSize }, LColor(ThemeToken::TextPrimary));
    }
}

void SearchField::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    if (!m_Text.empty() && ClearRect().Contains(event.position)) {
        m_Text.clear();
        if (m_OnTextChanged) {
            m_OnTextChanged(m_Text);
        }
        InvalidateUI();
        return;
    }
    m_CaretBlink = 0.0f;
    m_ShowCaret = true;
}

void SearchField::OnKeyDown(const KeyEvent& event) {
    if (!m_Focused) {
        return;
    }
    bool changed = false;
    if (event.key == we::platform::KeyCode::Backspace) {
        if (!m_Text.empty()) {
            EraseLastUtf8Codepoint(m_Text);
            changed = true;
        }
    } else if (event.key == we::platform::KeyCode::Escape) {
        if (!m_Text.empty()) {
            m_Text.clear();
            changed = true;
        }
    } else {
        const char typed = KeyCodeToChar(event.key, event.shiftDown);
        if (typed != '\0' && m_Text.size() < 128) {
            m_Text.push_back(typed);
            changed = true;
        }
    }
    if (changed && m_OnTextChanged) {
        m_OnTextChanged(m_Text);
        InvalidateUI();
    }
}

void SearchField::OnFocus() {
    m_Focused = true;
    m_CaretBlink = 0.0f;
    m_ShowCaret = true;
}

void SearchField::OnBlur() {
    m_Focused = false;
}

bool SearchField::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void SearchField::Tick(float deltaTime) {
    const float damp = LMetric(ThemeToken::HoverAnimationDamping);
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, damp);
    m_FocusAnim = Animator::Damp(m_FocusAnim, m_Focused ? 1.0f : 0.0f, damp);
    if (m_Focused) {
        m_CaretBlink += deltaTime;
        if (m_CaretBlink >= 0.53f) {
            m_CaretBlink = 0.0f;
            m_ShowCaret = !m_ShowCaret;
            InvalidateUI();
        }
    }
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ SegmentedControl Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

SegmentedControl::SegmentedControl(std::vector<std::string> labels, std::vector<std::string> icons)
    : m_Labels(std::move(labels))
    , m_Icons(std::move(icons)) {
    m_HoverAnims.assign(m_Labels.size(), 0.0f);
}

void SegmentedControl::SetSelected(int index) {
    if (index >= 0 && index < static_cast<int>(m_Labels.size())) {
        m_Selected = index;
        InvalidateUI();
    }
}

Size SegmentedControl::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    const float segW = 36.0f * s;
    m_DesiredSize = Size{ segW * static_cast<float>(m_Labels.size()) + 4.0f * s, kSegmentH * s };
    return m_DesiredSize;
}

void SegmentedControl::Arrange(const Rect& allottedRect) {
    const float h = kSegmentH * LScale();
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        m_DesiredSize.width,
        h
    };
}

Rect SegmentedControl::SegmentRect(int index) const {
    const float s = LScale();
    const float inner = m_Geometry.width - 4.0f * s;
    const float segW = inner / static_cast<float>(std::max(1, static_cast<int>(m_Labels.size())));
    return Rect{
        m_Geometry.x + 2.0f * s + segW * static_cast<float>(index),
        m_Geometry.y + 2.0f * s,
        segW,
        m_Geometry.height - 4.0f * s
    };
}

int SegmentedControl::HitSegment(const Point& p) const {
    for (int i = 0; i < static_cast<int>(m_Labels.size()); ++i) {
        if (SegmentRect(i).Contains(p)) {
            return i;
        }
    }
    return -1;
}

void SegmentedControl::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(ThemeToken::CornerRadiusSmall) * s;
    context.DrawRoundedRect(m_Geometry, LColor(ThemeToken::InputBackground), radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ThemeToken::BorderDefault), 1.0f, radius);

    for (int i = 0; i < static_cast<int>(m_Labels.size()); ++i) {
        const Rect r = SegmentRect(i);
        const bool selected = i == m_Selected;
        if (selected) {
            context.DrawRoundedRect(r, LColor(ThemeToken::SelectedBackground), radius - 1.0f);
        } else if (m_HoverAnims[static_cast<std::size_t>(i)] > 0.01f) {
            context.DrawRoundedRect(
                r,
                Color::Lerp(Color::Transparent(), LColor(ThemeToken::HoverBackground), m_HoverAnims[static_cast<std::size_t>(i)]),
                radius - 1.0f);
        }

        const float iconSize = 14.0f * s;
        const std::string& icon = (i < static_cast<int>(m_Icons.size()))
            ? m_Icons[static_cast<std::size_t>(i)]
            : m_Labels[static_cast<std::size_t>(i)];
        IconPainter::DrawIcon(
            context,
            icon,
            Rect{ r.x + (r.width - iconSize) * 0.5f, r.y + (r.height - iconSize) * 0.5f, iconSize, iconSize },
            selected ? LColor(ThemeToken::TextPrimary) : LColor(ThemeToken::IconSecondary));
    }
}

void SegmentedControl::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const int hit = HitSegment(event.position);
    if (hit >= 0 && hit != m_Selected) {
        m_Selected = hit;
        if (m_OnChanged) {
            m_OnChanged(m_Selected);
        }
        InvalidateUI();
    }
}

void SegmentedControl::OnMouseMove(const MouseEvent& event) {
    m_Hovered = HitSegment(event.position);
}

bool SegmentedControl::ShowsPointerCursor(const Point& position) const {
    return HitSegment(position) >= 0;
}

void SegmentedControl::Tick(float deltaTime) {
    const float damp = LMetric(ThemeToken::HoverAnimationDamping);
    for (int i = 0; i < static_cast<int>(m_HoverAnims.size()); ++i) {
        const bool hovered = i == m_Hovered && i != m_Selected;
        m_HoverAnims[static_cast<std::size_t>(i)] =
            Animator::Damp(m_HoverAnims[static_cast<std::size_t>(i)], hovered ? 1.0f : 0.0f, damp);
    }
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ StatusFooter Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

void StatusFooter::SetStatus(std::string status) {
    m_Status = std::move(status);
    InvalidateUI();
}

void StatusFooter::SetEngineLabel(std::string label) {
    m_EngineLabel = std::move(label);
    InvalidateUI();
}

void StatusFooter::SetSdkSummary(std::string summary) {
    m_SdkSummary = std::move(summary);
    InvalidateUI();
}

Size StatusFooter::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, kLauncherFooterH * LScale() };
    return m_DesiredSize;
}

void StatusFooter::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void StatusFooter::Paint(PaintContext& context) {
    const float s = LScale();
    context.DrawRect(m_Geometry, LColor(ThemeToken::FooterBackground));
    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, 1.0f * s },
        LColor(ThemeToken::Separator));

    const float textSize = LMetric(ThemeToken::TextSizeCaption) * s;
    const float pad = kLauncherContentPadX * s;
    const float textY = m_Geometry.y + (m_Geometry.height - textSize) * 0.5f;

    context.DrawText(m_Status, Point{ m_Geometry.x + pad, textY }, LColor(ThemeToken::TextMuted), textSize);

    std::string right;
    if (!m_EngineLabel.empty()) {
        right = m_EngineLabel;
    }
    if (!m_SdkSummary.empty()) {
        if (!right.empty()) {
            right += "  ·  ";
        }
        right += m_SdkSummary;
    }
    if (!right.empty()) {
        const float rightW = ApproxTextWidth(right, textSize);
        context.DrawText(
            right,
            Point{ m_Geometry.x + m_Geometry.width - pad - rightW, textY },
            LColor(ThemeToken::TextMuted),
            textSize);
    }
}

// Ã¢â€â‚¬Ã¢â€â‚¬ SectionCard Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

SectionCard::SectionCard() {
    m_HeaderHeight = 32.0f;
}

void SectionCard::SetContent(const std::shared_ptr<Widget>& content) {
    ClearChildren();
    m_Content = content;
    if (m_Content) {
        AddChild(m_Content);
    }
}

Size SectionCard::Measure(const Size& availableSize) {
    const float s = LScale();
    const float pad = LMetric(ThemeToken::Space3) * s;
    const float header = m_HeaderHeight * s;
    Size contentAvail{
        std::max(0.0f, availableSize.width - pad * 2.0f),
        std::max(0.0f, availableSize.height - header - pad * 2.0f)
    };
    Size contentSize{ 0.0f, 0.0f };
    if (m_Content) {
        contentSize = m_Content->Measure(contentAvail);
    }
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : contentSize.width + pad * 2.0f,
        header + contentSize.height + pad * 2.0f
    };
    return m_DesiredSize;
}

void SectionCard::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float s = LScale();
    const float pad = LMetric(ThemeToken::Space3) * s;
    const float header = m_HeaderHeight * s;
    if (m_Content) {
        m_Content->Arrange(Rect{
            allottedRect.x + pad,
            allottedRect.y + header + pad * 0.5f,
            std::max(0.0f, allottedRect.width - pad * 2.0f),
            std::max(0.0f, allottedRect.height - header - pad * 1.5f)
        });
    }
}

void SectionCard::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(ThemeToken::CornerRadiusMedium) * s;
    context.DrawRoundedRect(m_Geometry, LColor(ThemeToken::PanelBackground), radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ThemeToken::BorderDefault), 1.0f, radius);

    const float pad = LMetric(ThemeToken::Space3) * s;
    const float titleSize = LMetric(ThemeToken::TextSizeTabs) * s;
    context.DrawText(
        m_Title,
        Point{ m_Geometry.x + pad, m_Geometry.y + (m_HeaderHeight * s - titleSize) * 0.5f },
        LColor(ThemeToken::TextPrimary),
        titleSize,
        true);

    if (m_Content && m_Content->IsVisible()) {
        m_Content->Paint(context);
    }
}

void SectionCard::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ EmptyStatePanel Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

EmptyStatePanel::EmptyStatePanel(std::string title, std::string subtitle, const char* iconName)
    : m_Title(std::move(title))
    , m_Subtitle(std::move(subtitle))
    , m_Icon(iconName && iconName[0] ? iconName : Icons::Cube3DName) {
}

void EmptyStatePanel::SetPrimaryAction(std::string label, const char* icon, std::function<void()> onClick) {
    m_PrimaryLabel = std::move(label);
    m_PrimaryIcon = icon;
    m_OnPrimary = std::move(onClick);
    InvalidateUI();
}

void EmptyStatePanel::SetSecondaryAction(std::string label, const char* icon, std::function<void()> onClick) {
    m_SecondaryLabel = std::move(label);
    m_SecondaryIcon = icon;
    m_OnSecondary = std::move(onClick);
    InvalidateUI();
}

Size EmptyStatePanel::Measure(const Size& availableSize) {
    const float s = LScale();
    // Stretch to fill the host so the page never shows a large empty void.
    const float h = availableSize.height > 0.0f ? availableSize.height : 320.0f * s;
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 560.0f * s,
        h
    };
    return m_DesiredSize;
}

void EmptyStatePanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    const float s = LScale();
    const float btnH = LMetric(ThemeToken::HeaderControlHeight) * s;
    const float gap = LMetric(ThemeToken::Space2) * s;
    const bool hasPrimary = !m_PrimaryLabel.empty();
    const bool hasSecondary = !m_SecondaryLabel.empty();

    const float primaryW = hasPrimary
        ? std::max(140.0f * s, ApproxTextWidth(m_PrimaryLabel, LMetric(ThemeToken::TextSizeToolbar) * s) + 48.0f * s)
        : 0.0f;
    const float secondaryW = hasSecondary
        ? std::max(160.0f * s, ApproxTextWidth(m_SecondaryLabel, LMetric(ThemeToken::TextSizeToolbar) * s) + 48.0f * s)
        : 0.0f;

    float totalW = primaryW + secondaryW;
    if (hasPrimary && hasSecondary) {
        totalW += gap;
    }

    // Vertically center the action row under the illustration block.
    const float blockH = 220.0f * s;
    const float blockTop = m_Geometry.y + std::max(24.0f * s, (m_Geometry.height - blockH) * 0.5f);
    const float y = blockTop + 168.0f * s;
    float x = m_Geometry.x + (m_Geometry.width - totalW) * 0.5f;

    m_PrimaryRect = {};
    m_SecondaryRect = {};
    if (hasPrimary) {
        m_PrimaryRect = Rect{ x, y, primaryW, btnH };
        x += primaryW + gap;
    }
    if (hasSecondary) {
        m_SecondaryRect = Rect{ x, y, secondaryW, btnH };
    }
}

EmptyStatePanel::HitZone EmptyStatePanel::HitTest(const Point& p) const {
    if (!m_PrimaryLabel.empty() && m_PrimaryRect.Contains(p)) {
        return HitZone::Primary;
    }
    if (!m_SecondaryLabel.empty() && m_SecondaryRect.Contains(p)) {
        return HitZone::Secondary;
    }
    return HitZone::None;
}

void EmptyStatePanel::Paint(PaintContext& context) {
    const float s = LScale();
    // Fill surface so the host never reads as a black void.
    context.DrawRect(m_Geometry, LColor(ThemeToken::PanelContentBackground));

    const float cx = m_Geometry.x + m_Geometry.width * 0.5f;
    const float blockH = 220.0f * s;
    const float blockTop = m_Geometry.y + std::max(24.0f * s, (m_Geometry.height - blockH) * 0.5f);

    const float circle = 72.0f * s;
    const float circleY = blockTop;
    const Rect circleRect{ cx - circle * 0.5f, circleY, circle, circle };
    context.DrawRoundedRect(circleRect, LColor(ThemeToken::PanelBackground), circle * 0.5f);
    Color ring = LColor(ThemeToken::BorderDefault);
    ring.a *= 0.85f;
    context.DrawRoundedRectOutline(circleRect, ring, 1.0f, circle * 0.5f);

    const float iconSize = 32.0f * s;
    IconPainter::DrawIcon(
        context,
        m_Icon ? m_Icon : Icons::PackageName,
        Rect{
            cx - iconSize * 0.5f,
            circleY + (circle - iconSize) * 0.5f,
            iconSize,
            iconSize
        },
        LColor(ThemeToken::IconAccent));

    const float titleSize = LMetric(ThemeToken::TextSizeHeader) * s;
    const float titleY = circleY + circle + LMetric(ThemeToken::Space3) * s;
    const float titleW = ApproxTextWidth(m_Title, titleSize);
    context.DrawText(
        m_Title,
        Point{ cx - titleW * 0.5f, titleY },
        LColor(ThemeToken::TextPrimary),
        titleSize,
        true);

    const float subSize = LMetric(ThemeToken::TextSizeBody) * s;
    const float subY = titleY + titleSize + LMetric(ThemeToken::Space2) * s;
    const float subW = ApproxTextWidth(m_Subtitle, subSize);
    context.DrawText(
        m_Subtitle,
        Point{ cx - subW * 0.5f, subY },
        LColor(ThemeToken::TextSecondary),
        subSize);

    auto paintButton = [&](const Rect& r, const std::string& label, const char* icon, bool primary, bool hovered, bool pressed) {
        if (r.width <= 0.0f) {
            return;
        }
        const float radius = LMetric(ThemeToken::CornerRadiusSmall) * s;
        Color bg = primary
            ? LColor(ThemeToken::ButtonPrimaryBackground)
            : LColor(ThemeToken::ActiveBackground);
        if (primary) {
            if (pressed) {
                bg = LColor(ThemeToken::ButtonPrimaryPressed);
            } else if (hovered) {
                bg = Color::Lerp(bg, LColor(ThemeToken::ButtonPrimaryHover), 0.85f);
            }
        } else if (hovered) {
            bg = Color::Lerp(bg, LColor(ThemeToken::HoverBackground), 0.75f);
        }
        context.DrawRoundedRect(r, bg, radius);
        context.DrawRoundedRectOutline(
            r,
            primary ? LColor(ThemeToken::AccentPrimary) : LColor(ThemeToken::BorderDefault),
            1.0f,
            radius);

        const float glyph = 14.0f * s;
        const float textSize = LMetric(ThemeToken::TextSizeToolbar) * s;
        float contentW = ApproxTextWidth(label, textSize);
        if (icon && icon[0]) {
            contentW += glyph + 6.0f * s;
        }
        float x = r.x + (r.width - contentW) * 0.5f;
        const Color fg = LColor(ThemeToken::TextPrimary);
        if (icon && icon[0]) {
            IconPainter::DrawIcon(
                context,
                icon,
                Rect{ x, r.y + (r.height - glyph) * 0.5f, glyph, glyph },
                fg);
            x += glyph + 6.0f * s;
        }
        context.DrawText(label, Point{ x, r.y + (r.height - textSize) * 0.5f }, fg, textSize);
    };

    paintButton(
        m_PrimaryRect,
        m_PrimaryLabel,
        m_PrimaryIcon,
        true,
        m_Hover == HitZone::Primary,
        m_Pressed == HitZone::Primary);
    paintButton(
        m_SecondaryRect,
        m_SecondaryLabel,
        m_SecondaryIcon,
        false,
        m_Hover == HitZone::Secondary,
        m_Pressed == HitZone::Secondary);
}

void EmptyStatePanel::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = HitTest(event.position);
    }
}

void EmptyStatePanel::OnMouseMove(const MouseEvent& event) {
    const HitZone next = HitTest(event.position);
    if (next != m_Hover) {
        m_Hover = next;
        InvalidateUI();
    }
    m_Hovered = m_Hover != HitZone::None;
}

void EmptyStatePanel::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const HitZone hit = HitTest(event.position);
    if (hit == m_Pressed) {
        if (hit == HitZone::Primary && m_OnPrimary) {
            m_OnPrimary();
        } else if (hit == HitZone::Secondary && m_OnSecondary) {
            m_OnSecondary();
        }
    }
    m_Pressed = HitZone::None;
}

bool EmptyStatePanel::ShowsPointerCursor(const Point& position) const {
    return HitTest(position) != HitZone::None;
}

void EmptyStatePanel::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

// â”€â”€ CompactSearchField â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

CompactSearchField::CompactSearchField(std::string placeholder)
    : m_Placeholder(std::move(placeholder)) {
}

void CompactSearchField::SetText(std::string text) {
    if (m_Text == text) {
        return;
    }
    m_Text = std::move(text);
    InvalidateUI();
}

void CompactSearchField::AppendCodepoint(char32_t codepoint) {
    if (!m_Focused) {
        return;
    }
    if (AppendUtf8(m_Text, codepoint) && m_OnChanged) {
        m_OnChanged(m_Text);
    }
    InvalidateUI();
}

Size CompactSearchField::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    m_DesiredSize = Size{
        220.0f * s,
        LMetric(ThemeToken::HeaderControlHeight) * s
    };
    return m_DesiredSize;
}

void CompactSearchField::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void CompactSearchField::Paint(PaintContext& context) {
    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, false, m_Focused, false };
    ControlChrome::PaintInputFrame(context, m_Geometry, state);

    const float s = LScale();
    const float iconSize = 14.0f * s;
    const float pad = 8.0f * s;
    IconPainter::DrawIcon(
        context,
        Icons::SearchName,
        Rect{
            m_Geometry.x + pad,
            m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f,
            iconSize,
            iconSize
        },
        LColor(ThemeToken::IconSecondary));

    const float textSize = LMetric(ThemeToken::TextSizeCaption) * s;
    const bool empty = m_Text.empty();
    const std::string& draw = empty ? m_Placeholder : m_Text;
    context.DrawText(
        draw,
        Point{
            m_Geometry.x + pad + iconSize + 6.0f * s,
            m_Geometry.y + (m_Geometry.height - textSize) * 0.5f
        },
        empty ? LColor(ThemeToken::SearchPlaceholder) : LColor(ThemeToken::TextPrimary),
        textSize);
}

void CompactSearchField::OnMouseDown(const MouseEvent& event) {
    (void)event;
}

void CompactSearchField::OnKeyDown(const KeyEvent& event) {
    if (!m_Focused) {
        return;
    }
    if (event.key == we::platform::KeyCode::Backspace) {
        if (!m_Text.empty()) {
            EraseLastUtf8Codepoint(m_Text);
            if (m_OnChanged) {
                m_OnChanged(m_Text);
            }
            InvalidateUI();
        }
    } else if (event.key == we::platform::KeyCode::Escape) {
        if (!m_Text.empty()) {
            m_Text.clear();
            if (m_OnChanged) {
                m_OnChanged(m_Text);
            }
            InvalidateUI();
        }
    }
}

void CompactSearchField::OnFocus() {
    m_Focused = true;
    InvalidateUI();
}

void CompactSearchField::OnBlur() {
    m_Focused = false;
    InvalidateUI();
}

void CompactSearchField::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered ? 1.0f : 0.0f,
        LMetric(ThemeToken::HoverAnimationDamping));
    Widget::Tick(deltaTime);
}

bool CompactSearchField::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

// â”€â”€ ModalOverlay â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void ModalOverlay::SetDialog(const std::shared_ptr<Widget>& dialog) {
    ClearChildren();
    m_Dialog = dialog;
    if (m_Dialog) {
        AddChild(m_Dialog);
    }
    InvalidateUI();
}

Size ModalOverlay::Measure(const Size& availableSize) {
    if (m_Dialog) {
        const float s = LScale();
        m_Dialog->Measure(Size{ m_DialogWidth * s, availableSize.height * 0.85f });
    }
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void ModalOverlay::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (!m_Dialog) {
        return;
    }
    const float s = LScale();
    const float maxW = std::min(m_DialogWidth * s, allottedRect.width * 0.92f);
    Size desired = m_Dialog->GetDesiredSize();
    const float w = maxW;
    const float h = std::min(std::max(desired.height, 200.0f * s), allottedRect.height * 0.88f);
    m_Dialog->Arrange(Rect{
        allottedRect.x + (allottedRect.width - w) * 0.5f,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        w,
        h
    });
}

void ModalOverlay::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, LColor(ThemeToken::ModalScrim));
    if (m_Dialog && m_Dialog->IsVisible()) {
        m_Dialog->Paint(context);
    }
}

void ModalOverlay::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Dialog && !m_Dialog->GetGeometry().Contains(event.position)) {
        if (m_OnScrimClicked) {
            m_OnScrimClicked();
        }
    }
}

void ModalOverlay::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

} // namespace we::programs::welauncher
