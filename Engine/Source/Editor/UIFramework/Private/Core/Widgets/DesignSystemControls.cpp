#include "Core/Widgets/DesignSystemControls.h"

#include "Core/Animator.h"
#include "Core/ControlChrome.h"
#include "Core/Icon.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeManager.h"

#include <algorithm>

namespace WindEffects::Editor::UI {

DesignButton::DesignButton(std::string label, StyleRole role, const char* icon)
    : m_Label(std::move(label))
    , m_Role(role)
    , m_Icon(icon) {
}

Size DesignButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const ResolvedStyle style = ThemeManager::Get().Resolve(m_Role);
    const float pad = ResolveThemeMetric(ThemeToken::ButtonPaddingHorizontal);
    const float textW = static_cast<float>(m_Label.size()) * style.fontSize * 0.55f;
    const float iconW = m_Icon ? (style.iconSize + ResolveThemeMetric(ThemeToken::Space1)) : 0.0f;
    m_DesiredSize = Size{ textW + iconW + pad * 2.0f, style.height > 0.0f ? style.height : ResolveThemeMetric(ThemeToken::ButtonHeight) };
    return m_DesiredSize;
}

void DesignButton::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void DesignButton::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    const ResolvedStyle style = ThemeManager::Get().Resolve(m_Role);
    ControlChrome::InteractionState state{
        m_HoverAnim,
        m_PressAnim,
        false,
        m_Focused,
        !m_Enabled
    };

    if (m_Role == StyleRole::ButtonGhost) {
        ControlChrome::PaintGhostButton(context, m_Geometry, style, state);
    } else if (m_Role == StyleRole::ButtonDanger) {
        ControlChrome::PaintDangerButton(context, m_Geometry, style, state);
    } else {
        ControlChrome::PaintFilledButton(context, m_Geometry, style, state);
    }

    Color fg = style.foreground;
    if (!m_Enabled) {
        fg = ResolveThemeColor(ThemeToken::TextDisabled);
    }
    float x = m_Geometry.x + ResolveThemeMetric(ThemeToken::ButtonPaddingHorizontal);
    if (m_Icon) {
        IconPainter::DrawIcon(
            context,
            m_Icon,
            Rect{
                x,
                m_Geometry.y + (m_Geometry.height - style.iconSize) * 0.5f,
                style.iconSize,
                style.iconSize
            },
            fg);
        x += style.iconSize + ResolveThemeMetric(ThemeToken::Space1);
    }
    context.DrawText(
        m_Label,
        Point{ x, m_Geometry.y + (m_Geometry.height - style.fontSize) * 0.5f },
        fg,
        style.fontSize,
        style.bold);
}

void DesignButton::OnMouseDown(const MouseEvent& event) {
    if (m_Enabled && event.button == MouseButton::Left) {
        m_Pressed = true;
    }
}

void DesignButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Enabled && m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void DesignButton::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && m_Enabled ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, ControlChrome::PressDamping());
    Widget::Tick(deltaTime);
}

IconButton::IconButton(const char* icon)
    : m_Icon(icon) {
}

Size IconButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = ThemeManager::Get().Resolve(StyleRole::IconButton).height;
    m_DesiredSize = Size{ s, s };
    return m_DesiredSize;
}

void IconButton::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void IconButton::Paint(PaintContext& context) {
    ControlChrome::InteractionState state{ m_HoverAnim, m_PressAnim, m_Active, m_Focused, false };
    ControlChrome::PaintIconButtonFrame(context, m_Geometry, state, m_Active);
    if (m_Icon) {
        const ResolvedStyle style = ThemeManager::Get().Resolve(
            m_Active ? StyleRole::IconButtonPressed : StyleRole::IconButton);
        Color icon = m_Active
            ? ResolveThemeColor(ThemeToken::AccentPrimary)
            : ResolveThemeIconForState(m_HoverAnim > 0.5f, m_Active);
        IconPainter::DrawIcon(
            context,
            m_Icon,
            Rect{
                m_Geometry.x + (m_Geometry.width - style.iconSize) * 0.5f,
                m_Geometry.y + (m_Geometry.height - style.iconSize) * 0.5f,
                style.iconSize,
                style.iconSize
            },
            icon);
    }
}

void IconButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = true;
    }
}

void IconButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void IconButton::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, ControlChrome::PressDamping());
    Widget::Tick(deltaTime);
}

Size Card::Measure(const Size& availableSize) {
    float contentH = 0.0f;
    float contentW = availableSize.width > 0.0f ? availableSize.width : 240.0f;
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::Card);
    for (auto& child : GetChildren()) {
        if (child && child->IsVisible()) {
            const Size cs = child->Measure(Size{
                std::max(0.0f, contentW - style.padding.left - style.padding.right),
                availableSize.height
            });
            contentH += cs.height;
        }
    }
    m_DesiredSize = Size{
        contentW,
        contentH + style.padding.top + style.padding.bottom
    };
    return m_DesiredSize;
}

void Card::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::Card);
    float y = allottedRect.y + style.padding.top;
    const float innerW = allottedRect.width - style.padding.left - style.padding.right;
    for (auto& child : GetChildren()) {
        if (!child || !child->IsVisible()) {
            continue;
        }
        const Size cs = child->GetDesiredSize();
        child->Arrange(Rect{ allottedRect.x + style.padding.left, y, innerW, cs.height });
        y += cs.height;
    }
}

void Card::Paint(PaintContext& context) {
    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, false, false, false };
    ControlChrome::PaintCard(context, m_Geometry, state);
    for (auto& child : GetChildren()) {
        if (child && child->IsVisible()) {
            child->Paint(context);
        }
    }
}

void Card::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

SectionHeader::SectionHeader(std::string title, std::string subtitle)
    : m_Title(std::move(title))
    , m_Subtitle(std::move(subtitle)) {
}

Size SectionHeader::Measure(const Size& availableSize) {
    const ResolvedStyle header = ThemeManager::Get().Resolve(StyleRole::SectionHeader);
    const ResolvedStyle caption = ThemeManager::Get().Resolve(StyleRole::TextCaption);
    float h = header.fontSize + ResolveThemeMetric(ThemeToken::Space2);
    if (!m_Subtitle.empty()) {
        h += caption.fontSize + ResolveThemeMetric(ThemeToken::Space1);
    }
    m_DesiredSize = Size{ availableSize.width > 0.0f ? availableSize.width : 320.0f, h };
    return m_DesiredSize;
}

void SectionHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void SectionHeader::Paint(PaintContext& context) {
    ControlChrome::PaintSectionHeader(context, m_Geometry, m_Title, m_Subtitle);
}

PropertyRow::PropertyRow(std::string label, std::string value)
    : m_Label(std::move(label))
    , m_Value(std::move(value)) {
}

Size PropertyRow::Measure(const Size& availableSize) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::PropertyRow);
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 320.0f,
        style.height > 0.0f ? style.height : 28.0f
    };
    return m_DesiredSize;
}

void PropertyRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void PropertyRow::Paint(PaintContext& context) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::PropertyRow);
    const float caption = ThemeManager::Get().Resolve(StyleRole::TextCaption).fontSize;
    context.DrawText(
        m_Label,
        Point{ m_Geometry.x, m_Geometry.y + 2.0f },
        ResolveThemeColor(ThemeToken::TextMuted),
        caption);
    context.DrawText(
        m_Value.empty() ? "—" : m_Value,
        Point{ m_Geometry.x, m_Geometry.y + caption + 4.0f },
        style.foreground,
        style.fontSize);
}

SearchBoxControl::SearchBoxControl(std::string placeholder)
    : m_Placeholder(std::move(placeholder)) {
}

void SearchBoxControl::SetText(std::string text) {
    m_Text = std::move(text);
    if (m_OnChanged) {
        m_OnChanged(m_Text);
    }
}

Size SearchBoxControl::Measure(const Size& availableSize) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::SearchBox);
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 220.0f,
        style.height > 0.0f ? style.height : ResolveThemeMetric(ThemeToken::SearchBoxHeight)
    };
    return m_DesiredSize;
}

void SearchBoxControl::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void SearchBoxControl::Paint(PaintContext& context) {
    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, false, m_Focused, false };
    ControlChrome::PaintInputFrame(context, m_Geometry, state);
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::SearchBox);
    const float pad = ResolveThemeMetric(ThemeToken::Space2);
    const bool empty = m_Text.empty();
    context.DrawText(
        empty ? m_Placeholder : m_Text,
        Point{ m_Geometry.x + pad, m_Geometry.y + (m_Geometry.height - style.fontSize) * 0.5f },
        empty ? ResolveThemeColor(ThemeToken::SearchPlaceholder) : style.foreground,
        style.fontSize);
}

void SearchBoxControl::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

void SearchBoxControl::OnFocus() {
    m_Focused = true;
}

void SearchBoxControl::OnBlur() {
    m_Focused = false;
}

SidebarItem::SidebarItem(std::string label, const char* icon)
    : m_Label(std::move(label))
    , m_Icon(icon) {
}

Size SidebarItem::Measure(const Size& availableSize) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::SidebarItem);
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 200.0f,
        style.height
    };
    return m_DesiredSize;
}

void SidebarItem::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void SidebarItem::Paint(PaintContext& context) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(
        m_Active ? StyleRole::SidebarItemActive : StyleRole::SidebarItem);
    ControlChrome::InteractionState state{ m_HoverAnim, m_Pressed ? 1.0f : 0.0f, m_Active, false, false };
    ResolvedStyle paintStyle = style;
    if (!m_Active && m_HoverAnim > 0.01f) {
        paintStyle.background = Color::Lerp(
            Color::Transparent(),
            ResolveThemeColor(ThemeToken::HoverBackground),
            m_HoverAnim);
    }
    context.DrawRoundedRect(m_Geometry, paintStyle.background, style.cornerRadius);
    if (m_Active) {
        context.DrawRect(
            Rect{ m_Geometry.x, m_Geometry.y + 6.0f, 2.0f, m_Geometry.height - 12.0f },
            ResolveThemeColor(ThemeToken::AccentPrimary));
    }
    float x = m_Geometry.x + ResolveThemeMetric(ThemeToken::Space2);
    if (m_Icon) {
        IconPainter::DrawIcon(
            context,
            m_Icon,
            Rect{
                x,
                m_Geometry.y + (m_Geometry.height - style.iconSize) * 0.5f,
                style.iconSize,
                style.iconSize
            },
            style.icon);
        x += style.iconSize + ResolveThemeMetric(ThemeToken::Space2);
    }
    context.DrawText(
        m_Label,
        Point{ x, m_Geometry.y + (m_Geometry.height - style.fontSize) * 0.5f },
        style.foreground,
        style.fontSize,
        style.bold);
}

void SidebarItem::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = true;
    }
}

void SidebarItem::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void SidebarItem::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && !m_Active ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

WindowHeader::WindowHeader(std::string title)
    : m_Title(std::move(title)) {
}

Size WindowHeader::Measure(const Size& availableSize) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::WindowHeader);
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 400.0f,
        style.height
    };
    return m_DesiredSize;
}

void WindowHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void WindowHeader::Paint(PaintContext& context) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::WindowHeader);
    context.DrawRect(m_Geometry, style.background);
    context.DrawText(
        m_Title,
        Point{
            m_Geometry.x + ResolveThemeMetric(ThemeToken::Space3),
            m_Geometry.y + (m_Geometry.height - style.fontSize) * 0.5f
        },
        style.foreground,
        style.fontSize);
}

Size TableRowBase::Measure(const Size& availableSize) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::TableRow);
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 480.0f,
        style.height
    };
    return m_DesiredSize;
}

void TableRowBase::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TableRowBase::Paint(PaintContext& context) {
    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, m_Selected, m_Focused, false };
    ControlChrome::PaintListRow(context, m_Geometry, state);
}

void TableRowBase::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && !m_Selected ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

} // namespace WindEffects::Editor::UI
