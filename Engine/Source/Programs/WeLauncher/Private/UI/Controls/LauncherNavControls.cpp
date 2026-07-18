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
NavSidebar::NavSidebar() {
    m_Items = {
        { LauncherPage::Projects, "Projects", Icons::OpenFolderName },
        { LauncherPage::Learn, "Learn", Icons::MediaPlayName },
        { LauncherPage::Engine, "Engine", Icons::BuildName },
        { LauncherPage::Settings, "Settings", Icons::SettingsName },
    };
    m_WidthAnim = kLauncherNavWidth;
    SetFlexShrink(0.0f);
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
    const float padH = kLauncherNavPadX * s;
    const float padTop = kLauncherNavPadTop * s;
    const float itemH = kLauncherNavItemH * s;
    const float gap = kLauncherNavItemGap * s;
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
    context.DrawRect(m_Geometry, LColor(ColorToken::PanelBackground));

    for (int i = 0; i < static_cast<int>(m_Items.size()); ++i) {
        auto& item = m_Items[static_cast<std::size_t>(i)];
        const Rect r = ItemRect(i);
        const bool active = item.page == m_Active;
        const float radius = 8.0f * s;

        // Active: solid accent fill, no outline / accent bar. Inactive: transparent.
        if (active || item.selectAnim > 0.01f) {
            Color bg = LColor(ColorToken::AccentPrimary);
            bg.a *= std::max(item.selectAnim, active ? 1.0f : 0.0f);
            context.DrawRoundedRect(r, bg, radius);
        } else if (item.hoverAnim > 0.01f) {
            Color bg = LColor(ColorToken::HoverBackground);
            bg.a *= item.hoverAnim;
            context.DrawRoundedRect(r, bg, radius);
        }

        const Color fg = active
            ? Color{ 1.0f, 1.0f, 1.0f, 1.0f }
            : LColor(ColorToken::TextSecondary);
        const Color iconColor = active
            ? Color{ 1.0f, 1.0f, 1.0f, 1.0f }
            : LColor(ColorToken::IconSecondary);

        const float iconSize = kLauncherIconPx * s;
        const float iconPad = kLauncherNavIconTextGap * s;
        const float iconX = r.x + (m_Collapsed ? (r.width - iconSize) * 0.5f : iconPad);
        IconPainter::DrawIcon(
            context,
            item.icon,
            Rect{
                std::round(iconX),
                std::round(r.y + (r.height - iconSize) * 0.5f),
                iconSize,
                iconSize
            },
            iconColor);

        if (!m_Collapsed) {
            const float textSize = LMetric(MetricToken::TextSizeBody) * s;
            context.DrawText(
                item.label,
                Point{
                    std::round(iconX + iconSize + kLauncherNavIconTextGap * s),
                    std::round(r.y + (r.height - textSize) * 0.5f)
                },
                fg,
                textSize,
                active);
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
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
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

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ SearchField ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬

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
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;

    Color bg = LColor(ColorToken::SearchBoxBackground);
    if (m_HoverAnim > 0.01f || m_FocusAnim > 0.01f) {
        bg = Color::Lerp(bg, LColor(ColorToken::HoverBackground), std::max(m_HoverAnim, m_FocusAnim) * 0.35f);
    }
    context.DrawRoundedRect(m_Geometry, bg, radius);

    Color border = LColor(ColorToken::BorderDefault);
    if (m_FocusAnim > 0.01f) {
        border = Color::Lerp(border, LColor(ColorToken::BorderFocus), m_FocusAnim);
    }
    context.DrawRoundedRectOutline(m_Geometry, border, 1.0f, radius);

    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(LMetric(MetricToken::IconSizeSearch)));
    const float pad = 10.0f * s;
    IconPainter::DrawIcon(
        context,
        Icons::SearchName,
        Rect{ m_Geometry.x + pad, m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f, iconSize, iconSize },
        LColor(ColorToken::IconSecondary));

    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    const float textX = m_Geometry.x + pad + iconSize + 8.0f * s;
    const float textY = m_Geometry.y + (m_Geometry.height - textSize) * 0.5f;
    if (m_Text.empty()) {
        context.DrawText(m_Placeholder, Point{ textX, textY }, LColor(ColorToken::SearchPlaceholder), textSize);
    } else {
        context.DrawText(m_Text, Point{ textX, textY }, LColor(ColorToken::TextPrimary), textSize);
        IconPainter::DrawIcon(context, Icons::XName, ClearRect(), LColor(ColorToken::IconSecondary));
    }

    if (m_Focused && m_ShowCaret) {
        const float caretX = textX + ApproxTextWidth(m_Text, textSize);
        context.DrawRect(Rect{ caretX, textY, 1.0f * s, textSize }, LColor(ColorToken::TextPrimary));
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
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
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

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ SegmentedControl ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬

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
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;
    context.DrawRoundedRect(m_Geometry, LColor(ColorToken::InputBackground), radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderDefault), 1.0f, radius);

    for (int i = 0; i < static_cast<int>(m_Labels.size()); ++i) {
        const Rect r = SegmentRect(i);
        const bool selected = i == m_Selected;
        if (selected) {
            context.DrawRoundedRect(r, LColor(ColorToken::SelectedBackground), radius - 1.0f);
        } else if (m_HoverAnims[static_cast<std::size_t>(i)] > 0.01f) {
            context.DrawRoundedRect(
                r,
                Color::Lerp(Color::Transparent(), LColor(ColorToken::HoverBackground), m_HoverAnims[static_cast<std::size_t>(i)]),
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
            selected ? LColor(ColorToken::TextPrimary) : LColor(ColorToken::IconSecondary));
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
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    for (int i = 0; i < static_cast<int>(m_HoverAnims.size()); ++i) {
        const bool hovered = i == m_Hovered && i != m_Selected;
        m_HoverAnims[static_cast<std::size_t>(i)] =
            Animator::Damp(m_HoverAnims[static_cast<std::size_t>(i)], hovered ? 1.0f : 0.0f, damp);
    }
    Widget::Tick(deltaTime);
}

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ StatusFooter ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬

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
    context.DrawRect(m_Geometry, LColor(ColorToken::FooterBackground));
    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, 1.0f * s },
        LColor(ColorToken::Separator));

    const float textSize = LMetric(MetricToken::TextSizeCaption) * s;
    const float pad = kLauncherContentPadX * s;
    const float iconPx = 14.0f * s;
    const float iconGap = 6.0f * s;
    const float textY = m_Geometry.y + (m_Geometry.height - textSize) * 0.5f;
    const float iconY = m_Geometry.y + (m_Geometry.height - iconPx) * 0.5f;
    const Color iconColor = LColor(ColorToken::IconSecondary);
    const Color textColor = LColor(ColorToken::TextCaption);

    float leftX = m_Geometry.x + pad;
    IconPainter::DrawIcon(
        context,
        Icons::InfoName,
        Rect{ leftX, iconY, iconPx, iconPx },
        iconColor);
    leftX += iconPx + iconGap;
    context.DrawText(m_Status, Point{ leftX, textY }, textColor, textSize);

    float rightX = m_Geometry.x + m_Geometry.width - pad;
    if (!m_SdkSummary.empty()) {
        const float sdkW = context.GetTextWidth(m_SdkSummary, textSize);
        rightX -= sdkW;
        context.DrawText(m_SdkSummary, Point{ rightX, textY }, textColor, textSize);
        rightX -= iconGap;
        rightX -= iconPx;
        IconPainter::DrawIcon(
            context,
            Icons::PackageName,
            Rect{ rightX, iconY, iconPx, iconPx },
            iconColor);
        rightX -= 14.0f * s;
    }
    if (!m_EngineLabel.empty()) {
        const float engW = context.GetTextWidth(m_EngineLabel, textSize);
        rightX -= engW;
        context.DrawText(m_EngineLabel, Point{ rightX, textY }, textColor, textSize);
        rightX -= iconGap;
        rightX -= iconPx;
        IconPainter::DrawIcon(
            context,
            Icons::BuildName,
            Rect{ rightX, iconY, iconPx, iconPx },
            iconColor);
    }
}

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ SectionCard ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬

} // namespace we::programs::welauncher
