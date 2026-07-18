#include "UI/Pages/Settings/SettingsViews.h"
#include "SettingsViewsHelpers.h"
#include "UI/Shell/LauncherHelpers.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Layout/Flex.h"
#include "Platform/PlatformSDK.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Text/Layout/TextStyle.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
using namespace we::runtime::kindui;
namespace we::programs::welauncher {
using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;
using namespace settings_detail;
ToggleSwitch::ToggleSwitch(bool on)
    : m_On(on)
    , m_Anim(on ? 1.0f : 0.0f) {
}

void ToggleSwitch::SetOn(bool on) {
    m_On = on;
    InvalidateUI();
}

Size ToggleSwitch::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    m_DesiredSize = Size{ 40.0f * s, 22.0f * s };
    return m_DesiredSize;
}

void ToggleSwitch::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x + allottedRect.width - m_DesiredSize.width,
        allottedRect.y + (allottedRect.height - m_DesiredSize.height) * 0.5f,
        m_DesiredSize.width,
        m_DesiredSize.height
    };
}

void ToggleSwitch::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = m_Geometry.height * 0.5f;
    Color off = LColor(ColorToken::InputBackground);
    Color on = LColor(ColorToken::AccentPrimary);
    Color track = Color::Lerp(off, on, m_Anim);
    context.DrawRoundedRect(m_Geometry, track, radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderDefault), 1.0f, radius);

    const float knob = m_Geometry.height - 4.0f * s;
    const float travel = m_Geometry.width - knob - 4.0f * s;
    Rect knobRect{
        m_Geometry.x + 2.0f * s + travel * m_Anim,
        m_Geometry.y + 2.0f * s,
        knob,
        knob
    };
    context.DrawRoundedRect(knobRect, LColor(ColorToken::TextPrimary), knob * 0.5f);
}

void ToggleSwitch::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Geometry.Contains(event.position)) {
        m_Pressed = true;
    }
}

void ToggleSwitch::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed && m_Geometry.Contains(event.position)) {
        m_On = !m_On;
        if (m_OnChanged) {
            m_OnChanged(m_On);
        }
        InvalidateUI();
    }
    m_Pressed = false;
}

bool ToggleSwitch::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void ToggleSwitch::Tick(float deltaTime) {
    (void)deltaTime;
    const float prev = m_Anim;
    m_Anim = Animator::Damp(m_Anim, m_On ? 1.0f : 0.0f, 18.0f);
    if (std::fabs(m_Anim - prev) > 0.001f) {
        InvalidateUI();
    }
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ SettingsCheckBox Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

SettingsCheckBox::SettingsCheckBox(bool checked)
    : m_Checked(checked) {
}

void SettingsCheckBox::SetChecked(bool checked) {
    m_Checked = checked;
    InvalidateUI();
}

Size SettingsCheckBox::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    m_DesiredSize = Size{ 18.0f * s, 18.0f * s };
    return m_DesiredSize;
}

void SettingsCheckBox::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x + allottedRect.width - m_DesiredSize.width,
        allottedRect.y + (allottedRect.height - m_DesiredSize.height) * 0.5f,
        m_DesiredSize.width,
        m_DesiredSize.height
    };
}

void SettingsCheckBox::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = 4.0f * s;
    Color bg = m_Checked ? LColor(ColorToken::AccentPrimary) : LColor(ColorToken::InputBackground);
    if (m_HoverAnim > 0.01f && !m_Checked) {
        bg = Color::Lerp(bg, LColor(ColorToken::HoverBackground), m_HoverAnim);
    }
    context.DrawRoundedRect(m_Geometry, bg, radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderDefault), 1.0f, radius);
    if (m_Checked) {
        IconPainter::DrawIcon(
            context,
            Icons::CheckName,
            Rect{
                m_Geometry.x + 2.0f * s,
                m_Geometry.y + 2.0f * s,
                m_Geometry.width - 4.0f * s,
                m_Geometry.height - 4.0f * s
            },
            LColor(ColorToken::TextPrimary));
    }
}

void SettingsCheckBox::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Geometry.Contains(event.position)) {
        m_Pressed = true;
    }
}

void SettingsCheckBox::OnMouseMove(const MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

void SettingsCheckBox::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed && m_Geometry.Contains(event.position)) {
        m_Checked = !m_Checked;
        if (m_OnChanged) {
            m_OnChanged(m_Checked);
        }
        InvalidateUI();
    }
    m_Pressed = false;
}

bool SettingsCheckBox::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void SettingsCheckBox::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered ? 1.0f : 0.0f,
        LMetric(MetricToken::HoverAnimationDamping));
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ SettingsDropdown Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

SettingsDropdown::SettingsDropdown(std::vector<std::string> options, int selected)
    : m_Options(std::move(options))
    , m_Selected(std::clamp(selected, 0, std::max(0, static_cast<int>(m_Options.size()) - 1))) {
}

void SettingsDropdown::SetSelected(int index) {
    if (index >= 0 && index < static_cast<int>(m_Options.size())) {
        m_Selected = index;
        InvalidateUI();
    }
}

const std::string& SettingsDropdown::SelectedLabel() const {
    static const std::string kEmpty;
    if (m_Options.empty()) {
        return kEmpty;
    }
    return m_Options[static_cast<std::size_t>(m_Selected)];
}

Size SettingsDropdown::Measure(const Size& availableSize) {
    const float s = LScale();
    float maxW = 80.0f * s;
    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    const float triggerH = LControlH() * s;
    const float itemH = LMetric(MetricToken::MenuItemHeight) * s;
    for (const auto& opt : m_Options) {
        maxW = std::max(maxW, ApproxTextWidth(opt, textSize) + 36.0f * s);
    }
    if (availableSize.width > maxW) {
        maxW = availableSize.width;
    }
    float h = triggerH;
    if (m_Open) {
        h += LMetric(MetricToken::Space1) * s + static_cast<float>(m_Options.size()) * itemH;
    }
    m_DesiredSize = Size{ maxW, h };
    return m_DesiredSize;
}

void SettingsDropdown::Arrange(const Rect& allottedRect) {
    const float s = LScale();
    const float triggerH = LControlH() * s;
    const float w = allottedRect.width > 0.0f ? allottedRect.width : m_DesiredSize.width;
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y,
        w,
        m_Open ? m_DesiredSize.height : triggerH
    };
}

Rect SettingsDropdown::MenuRect() const {
    const float s = LScale();
    const float triggerH = LControlH() * s;
    const float itemH = LMetric(MetricToken::MenuItemHeight) * s;
    return Rect{
        m_Geometry.x,
        m_Geometry.y + triggerH + LMetric(MetricToken::Space1) * 0.5f * s,
        m_Geometry.width,
        static_cast<float>(m_Options.size()) * itemH
    };
}

Rect SettingsDropdown::OptionRect(int index) const {
    const float s = LScale();
    const float itemH = LMetric(MetricToken::MenuItemHeight) * s;
    const Rect menu = MenuRect();
    return Rect{ menu.x, menu.y + static_cast<float>(index) * itemH, menu.width, itemH };
}

int SettingsDropdown::HitOption(const Point& p) const {
    if (!m_Open) {
        return -1;
    }
    for (int i = 0; i < static_cast<int>(m_Options.size()); ++i) {
        if (OptionRect(i).Contains(p)) {
            return i;
        }
    }
    return -1;
}

void SettingsDropdown::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;
    const float triggerH = LControlH() * s;
    Rect trigger{ m_Geometry.x, m_Geometry.y, m_Geometry.width, triggerH };

    Color bg = LColor(ColorToken::InputBackground);
    if (m_HoverAnim > 0.01f || m_Open) {
        bg = Color::Lerp(bg, LColor(ColorToken::HoverBackground), std::max(m_HoverAnim, m_Open ? 1.0f : 0.0f));
    }
    context.DrawRoundedRect(trigger, bg, radius);
    context.DrawRoundedRectOutline(
        trigger,
        m_Open ? LColor(ColorToken::BorderFocus) : LColor(ColorToken::BorderDefault),
        1.0f,
        radius);

    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    context.DrawText(
        SelectedLabel(),
        Point{ trigger.x + 10.0f * s, trigger.y + (triggerH - textSize) * 0.5f },
        InputValueTextColor(),
        textSize);
    IconPainter::DrawIcon(
        context,
        Icons::ChevronDownName,
        Rect{ trigger.x + trigger.width - 20.0f * s, trigger.y + (triggerH - 12.0f * s) * 0.5f, 12.0f * s, 12.0f * s },
        LColor(ColorToken::IconSecondary));

    if (m_Open) {
        const Rect menu = MenuRect();
        ControlChrome::PaintPopupSurface(context, menu);
        for (int i = 0; i < static_cast<int>(m_Options.size()); ++i) {
            const Rect opt = OptionRect(i);
            if (i == m_Selected || i == m_HoverOption) {
                context.DrawRoundedRect(opt, LColor(ColorToken::HoverBackground), radius - 1.0f);
            }
            context.DrawText(
                m_Options[static_cast<std::size_t>(i)],
                Point{ opt.x + 10.0f * s, opt.y + (opt.height - textSize) * 0.5f },
                i == m_Selected ? LColor(ColorToken::AccentPrimary) : InputValueTextColor(),
                textSize);
        }
    }
}

void SettingsDropdown::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const float s = LScale();
    Rect trigger{ m_Geometry.x, m_Geometry.y, m_Geometry.width, LControlH() * s };
    if (trigger.Contains(event.position)) {
        m_Open = !m_Open;
        InvalidateUI();
        return;
    }
    const int opt = HitOption(event.position);
    if (opt >= 0) {
        m_Selected = opt;
        m_Open = false;
        if (m_OnChanged) {
            m_OnChanged(m_Selected);
        }
        InvalidateUI();
    } else if (m_Open) {
        m_Open = false;
        InvalidateUI();
    }
}

void SettingsDropdown::OnMouseMove(const MouseEvent& event) {
    m_HoverOption = HitOption(event.position);
    m_Hovered = m_Geometry.Contains(event.position);
}

void SettingsDropdown::OnMouseUp(const MouseEvent& event) {
    (void)event;
}

bool SettingsDropdown::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void SettingsDropdown::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered ? 1.0f : 0.0f,
        LMetric(MetricToken::HoverAnimationDamping));
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ SettingsSegmented Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

SettingsSegmented::SettingsSegmented(std::vector<std::string> labels)
    : m_Labels(std::move(labels)) {
    m_HoverAnims.assign(m_Labels.size(), 0.0f);
}

void SettingsSegmented::SetSelected(int index) {
    if (index >= 0 && index < static_cast<int>(m_Labels.size())) {
        m_Selected = index;
        InvalidateUI();
    }
}

Size SettingsSegmented::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    const float textSize = LMetric(MetricToken::TextSizeCaption) * s;
    float w = 4.0f * s;
    for (const auto& label : m_Labels) {
        w += ApproxTextWidth(label, textSize) + 20.0f * s;
    }
    m_DesiredSize = Size{ std::max(120.0f * s, w), LControlH(ControlSize::Compact) * s };
    return m_DesiredSize;
}

void SettingsSegmented::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x + allottedRect.width - m_DesiredSize.width,
        allottedRect.y + (allottedRect.height - m_DesiredSize.height) * 0.5f,
        m_DesiredSize.width,
        m_DesiredSize.height
    };
}

Rect SettingsSegmented::SegmentRect(int index) const {
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

int SettingsSegmented::HitSegment(const Point& p) const {
    for (int i = 0; i < static_cast<int>(m_Labels.size()); ++i) {
        if (SegmentRect(i).Contains(p)) {
            return i;
        }
    }
    return -1;
}

void SettingsSegmented::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;
    context.DrawRoundedRect(m_Geometry, LColor(ColorToken::InputBackground), radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderDefault), 1.0f, radius);

    const float textSize = LMetric(MetricToken::TextSizeCaption) * s;
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
        const std::string& label = m_Labels[static_cast<std::size_t>(i)];
        const float tw = ApproxTextWidth(label, textSize);
        context.DrawText(
            label,
            Point{ r.x + (r.width - tw) * 0.5f, r.y + (r.height - textSize) * 0.5f },
            selected ? LColor(ColorToken::TextPrimary) : LColor(ColorToken::TextSecondary),
            textSize,
            selected);
    }
}

void SettingsSegmented::OnMouseDown(const MouseEvent& event) {
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

void SettingsSegmented::OnMouseMove(const MouseEvent& event) {
    m_Hovered = HitSegment(event.position);
}

bool SettingsSegmented::ShowsPointerCursor(const Point& position) const {
    return HitSegment(position) >= 0;
}

void SettingsSegmented::Tick(float deltaTime) {
    (void)deltaTime;
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    for (int i = 0; i < static_cast<int>(m_HoverAnims.size()); ++i) {
        m_HoverAnims[static_cast<std::size_t>(i)] = Animator::Damp(
            m_HoverAnims[static_cast<std::size_t>(i)],
            (i == m_Hovered && i != m_Selected) ? 1.0f : 0.0f,
            damp);
    }
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ SettingsTextField Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

} // namespace we::programs::welauncher
