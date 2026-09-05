#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include "KindUI/Core/Animator.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"
#include "Platform/Platform.h"

#include "KindUI/Rendering/IconMetrics.h"

#include "Text/Layout/TextStyle.h"

#include <algorithm>

namespace we::runtime::kindui {

DesignButton::DesignButton(std::string label, StyleRole role, WindIconRef icon)
    : m_Label(std::move(label))
    , m_Role(role)
    , m_Icon(icon) {
    SetFocusable(false);
    LayoutMetrics::ApplyButtonMinSize(*this, m_Role);
}

void DesignButton::SetLabel(std::string label) {
    m_Label = std::move(label);
    InvalidatePaint();
}

Size DesignButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const ResolvedStyle style = ThemeManager::Get().Resolve(m_Role);
    const float pad = ResolveMetric(MetricToken::Space2);
    const float textW = TextMetrics::MeasureWidth(m_Label, style.fontSize);
    const float iconW = m_Icon.IsValid() ? (style.iconSize + ResolveMetric(MetricToken::Space1)) : 0.0f;
    m_DesiredSize = Size{ textW + iconW + pad * 2.0f, style.height > 0.0f ? style.height : ResolveMetric(MetricToken::ButtonHeight) };
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
        !IsEnabled()
    };

    if (m_Role == StyleRole::ButtonGhost) {
        ControlChrome::PaintGhostButton(context, m_Geometry, style, state);
    } else if (m_Role == StyleRole::ButtonDanger) {
        ControlChrome::PaintDangerButton(context, m_Geometry, style, state);
    } else if (m_Role == StyleRole::ButtonPrimary) {
        ControlChrome::PaintFilledButton(
            context, m_Geometry, style, state, StyleRole::ButtonPrimary, StyleRole::ButtonPrimary);
    } else {
        ControlChrome::PaintFilledButton(context, m_Geometry, style, state);
    }

    Color fg = style.foreground;
    if (!IsEnabled()) {
        fg = ResolveColor(ColorToken::TextDisabled);
    }
    const float pad = ResolveMetric(MetricToken::Space2);
    const float textW = TextMetrics::MeasureWidth(m_Label, style.fontSize, style.bold);
    const float iconW = m_Icon.IsValid() ? (style.iconSize + ResolveMetric(MetricToken::Space1)) : 0.0f;
    const float contentW = textW + iconW;
    // Center label in wide full-width buttons (toggles / CTAs); left-align when compact.
    float x = m_Geometry.x + pad;
    if (m_Geometry.width > contentW + pad * 2.0f + ResolveMetric(MetricToken::Space2)) {
        x = m_Geometry.x + (m_Geometry.width - contentW) * 0.5f;
    }
    if (m_Icon.IsValid()) {
        const Rect iconRect{
            x,
            m_Geometry.y + (m_Geometry.height - style.iconSize) * 0.5f,
            style.iconSize,
            style.iconSize};
        IconPainter::Draw(context, m_Icon, iconRect);
        x += style.iconSize + ResolveMetric(MetricToken::Space1);
    }
    context.DrawText(
        m_Label,
        Point{ x, m_Geometry.y + (m_Geometry.height - style.fontSize) * 0.5f },
        fg,
        style.fontSize,
        style.bold ? we::runtime::text::layout::FontWeight::Medium
                   : we::runtime::text::layout::FontWeight::Regular);
}

void DesignButton::OnMouseDown(const MouseEvent& event) {
    if (IsEnabled() && event.button == MouseButton::Left) {
        SetPressed(true);
    }
}

void DesignButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        SetPressed(false);
        if (IsEnabled() && m_OnClicked) {
            m_OnClicked();
        }
    }
}

void DesignButton::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && IsEnabled() ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, ControlChrome::PressDamping());
    Widget::Tick(deltaTime);
}

IconButton::IconButton(WindIconRef icon)
    : m_Icon(icon) {
}

Size IconButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = ThemeManager::Get().Resolve(StyleRole::IconButton).height;
    m_DesiredSize = Size{ s, s };
    return m_DesiredSize;
}

void IconButton::Arrange(const Rect& allottedRect) {
    const float size = ThemeManager::Get().Resolve(StyleRole::IconButton).height;
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - size) * 0.5f,
        size,
        size
    };
}

void IconButton::Paint(PaintContext& context) {
    ControlChrome::InteractionState state{ m_HoverAnim, m_PressAnim, m_Active, m_Focused, false };
    if (m_Borderless) {
        ControlChrome::PaintBorderlessIconButton(context, m_Geometry, state);
    } else {
        ControlChrome::PaintIconButtonFrame(context, m_Geometry, state, m_Active);
    }
    if (m_Icon.IsValid()) {
        const ResolvedStyle style = ThemeManager::Get().Resolve(
            m_Active ? StyleRole::IconButtonPressed : StyleRole::IconButton);
        const float iconPx = style.iconSize;
        IconPainter::Draw(context, m_Icon, m_Geometry, static_cast<uint32_t>(iconPx));
    }
}

void IconButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        SetPressed(true);
    }
}

void IconButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        SetPressed(false);
        if (m_OnClicked) {
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
    (void)availableSize;
    const TypographySpec titleSpec = ResolveTypography(TypographyToken::SectionTitle);
    const TypographySpec subtitleSpec = ResolveTypography(TypographyToken::Subtitle);
    float h = titleSpec.lineHeightPx;
    float w = TextMetrics::MeasureWidth(m_Title, titleSpec.sizePx, titleSpec.bold);
    if (!m_Subtitle.empty()) {
        h += subtitleSpec.lineHeightPx + ResolveMetric(MetricToken::LabelHintGap);
        w = std::max(w, TextMetrics::MeasureWidth(m_Subtitle, subtitleSpec.sizePx, subtitleSpec.bold));
    } else {
        h += ResolveMetric(MetricToken::Space2);
    }
    // Report content-intrinsic width. Parent Flex Stretch expands on the cross axis;
    // claiming availableSize.width made every Column inside a Row overflow and shrink
    // all siblings to 0x0.
    m_DesiredSize = Size{ std::max(w, ResolveMetric(MetricToken::IconButtonSize) + ResolveMetric(MetricToken::Space2)), h };
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
    LayoutMetrics::ApplyFormRowMinSize(*this);
}

Size PropertyRow::Measure(const Size& availableSize) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::PropertyRow);
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 320.0f,
        style.height > 0.0f ? style.height : ResolveMetric(MetricToken::FormRowHeight)
    };
    return m_DesiredSize;
}

void PropertyRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void PropertyRow::Paint(PaintContext& context) {
    const auto layout = PropertyPanelChrome::LayoutPropertyRow(m_Geometry, 0);
    PropertyPanelChrome::PaintPropertyRowLabel(context, layout.label, m_Label, false);

    const TypographySpec valueSpec = ResolveTypography(TypographyToken::PropertyValue);
    const float fontSize = valueSpec.sizePx;
    const float textY = layout.value.y + (layout.value.height - fontSize) * 0.5f;
    context.DrawText(
        m_Value.empty() ? "—" : m_Value,
        Point{ layout.value.x, textY },
        valueSpec.color,
        fontSize,
        valueSpec.bold);
}

SearchBoxControl::SearchBoxControl(std::string placeholder)
    : m_Placeholder(std::move(placeholder)) {
    SetFocusable(true);
    SetStyleClass("SearchBar");
    LayoutMetrics::ApplyInputMinSize(*this);
}

void SearchBoxControl::SetToolbarInset(bool inset) {
    m_ToolbarInset = inset;
    if (inset) {
        const Size current = GetMinSize();
        SetMinSize({ current.width, LayoutMetrics::ToolbarSearchInputHeight() });
    } else {
        LayoutMetrics::ApplyInputMinSize(*this);
    }
    InvalidateLayout();
}

void SearchBoxControl::SetText(std::string text) {
    m_Text = std::move(text);
    if (m_OnChanged) {
        m_OnChanged(m_Text);
    }
}

Size SearchBoxControl::Measure(const Size& availableSize) {
    const float minW = m_MinSize.width > 0.0f
        ? m_MinSize.width
        : ResolveMetric(MetricToken::Space6) * 4.0f;
    float w = minW;
    if (m_FillWidth) {
        w = availableSize.width < 1.0e8f ? availableSize.width : minW;
    } else if (m_Width > 0.0f) {
        w = m_Width;
    } else {
        w = ResolveMetric(MetricToken::InputWidthLarge) * (std::max)(1.0f, DPIContext::GetScale());
    }
    const float h = m_ToolbarInset
        ? LayoutMetrics::ToolbarSearchInputHeight()
        : LayoutMetrics::SearchInputHeight();
    m_DesiredSize = Size{ w, h };
    return m_DesiredSize;
}

void SearchBoxControl::Arrange(const Rect& allottedRect) {
    m_Geometry = m_ToolbarInset
        ? LayoutMetrics::LayoutToolbarSearchInputRect(allottedRect)
        : LayoutMetrics::LayoutSearchInputRect(allottedRect);
}

void SearchBoxControl::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, false, m_Focused, false };
    ControlChrome::SearchFieldPaintOptions options{};
    options.toolbarFlat = m_ToolbarFlat;
    ControlChrome::PaintSearchField(
        context,
        m_Geometry,
        m_Placeholder,
        m_Text,
        state,
        m_Focused && !m_Text.empty(),
        options);
}

void SearchBoxControl::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}


void SearchBoxControl::OnKeyDown(const KeyEvent& event) {
    if (!m_Focused) {
        return;
    }

    bool changed = false;
    if (event.key == we::platform::KeyCode::Backspace) {
        if (!m_Text.empty()) {
            m_Text.pop_back();
            changed = true;
        }
    } else if (event.key == we::platform::KeyCode::Escape) {
        OnBlur();
    } else if (event.key == we::platform::KeyCode::Space) {
        m_Text += ' ';
        changed = true;
    } else {
        const char typedChar = KeyCodeToChar(event.key, event.shiftDown);
        if (typedChar != 0) {
            m_Text += typedChar;
            changed = true;
        }
    }

    if (changed) {
        InvalidatePaint();
        if (m_OnChanged) {
            m_OnChanged(m_Text);
        }
    }
}
void SearchBoxControl::OnFocus() {
    m_Focused = true;
}

void SearchBoxControl::OnBlur() {
    m_Focused = false;
}

PanelTab::PanelTab(std::string label)
    : m_Label(std::move(label)) {
    SetFocusable(false);
    SetMinSize({ 0.0f, ResolveMetric(MetricToken::PanelTabHeight) });
}

Size PanelTab::Measure(const Size& availableSize) {
    const float padH = ResolveMetric(MetricToken::Space3);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption);
    const float textW = TextMetrics::MeasureWidth(m_Label, fontSize, false);
    float w = textW + padH * 2.0f;
    if (availableSize.width > 0.0f) {
        w = std::max(w, availableSize.width);
    }
    m_DesiredSize = Size{ w, ResolveMetric(MetricToken::PanelTabHeight) };
    return m_DesiredSize;
}

void PanelTab::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void PanelTab::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    ControlChrome::InteractionState state{
        m_HoverAnim,
        m_Pressed ? 1.0f : 0.0f,
        m_Active,
        false,
        false
    };
    ControlChrome::PaintPanelTab(context, m_Geometry, m_Label, state);
}

void PanelTab::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = true;
    }
}

void PanelTab::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_OnClicked) {
            m_OnClicked();
        }
    }
}

void PanelTab::Tick(float deltaTime) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

SidebarItem::SidebarItem(std::string label, WindIconRef icon)
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
    (void)state;
    const Color fill = we::runtime::kindui::MixInteractiveSurface(
        style.background,
        m_Active ? 0.0f : m_HoverAnim,
        m_Pressed && !m_Active ? 1.0f : 0.0f,
        m_Active,
        false);
    context.DrawRoundedRect(m_Geometry, fill, style.cornerRadius);
    if (m_Active) {
        context.DrawRect(
            Rect{ m_Geometry.x, m_Geometry.y + 6.0f, 2.0f, m_Geometry.height - 12.0f },
            ResolveColor(ColorToken::AccentPrimary));
    }
    float x = m_Geometry.x + ResolveMetric(MetricToken::Space2);
    if (m_Icon.IsValid()) {
        const Rect iconRect{
            x,
            m_Geometry.y + (m_Geometry.height - style.iconSize) * 0.5f,
            style.iconSize,
            style.iconSize};
        IconPainter::Draw(context, m_Icon, iconRect);
        x += style.iconSize + ResolveMetric(MetricToken::Space2);
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
        SetPressed(true);
    }
}

void SidebarItem::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        SetPressed(false);
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
            m_Geometry.x + ResolveMetric(MetricToken::Space3),
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

std::shared_ptr<PrimaryButton> MakePrimaryAction(std::string label, WindIconRef icon) {
    return std::make_shared<PrimaryButton>(std::move(label), icon);
}

std::shared_ptr<SecondaryButton> MakeSecondaryAction(std::string label, WindIconRef icon) {
    return std::make_shared<SecondaryButton>(std::move(label), icon);
}

std::shared_ptr<PanelTab> MakePanelTab(std::string label) {
    return std::make_shared<PanelTab>(std::move(label));
}

} // namespace we::runtime::kindui
