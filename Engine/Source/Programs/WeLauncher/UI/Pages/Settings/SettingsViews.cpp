#include "UI/Pages/Settings/SettingsViews.h"

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
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>

using namespace we::runtime::kindui;

namespace we::programs::welauncher {

using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;

namespace {

float ApproxTextWidth(const std::string& text, float textSize) {
    return TextMetrics::MeasureWidth(text, textSize);
}

std::string ToLowerLocal(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

bool ContainsInsensitive(const std::string& haystack, const std::string& needleLower) {
    if (needleLower.empty()) {
        return true;
    }
    return ToLowerLocal(haystack).find(needleLower) != std::string::npos;
}

const char* kAccentPalette[] = {
    "#5B8DEF", "#6BCB9A", "#E0A35A", "#C97BDB", "#5AABB8", "#E07070", "#A0A8B8"
};

} // namespace

const char* SettingsCategoryTitle(SettingsCategory category) {
    switch (category) {
    case SettingsCategory::General: return "General";
    case SettingsCategory::Engine: return "Engine";
    case SettingsCategory::Storage: return "Storage";
    case SettingsCategory::FileAssociations: return "File Associations";
    case SettingsCategory::About: return "About";
    default: return "Settings";
    }
}

const char* SettingsCategoryKeywords(SettingsCategory category) {
    switch (category) {
    case SettingsCategory::General:
        return "general projects folder engine version recent limit open last start";
    case SettingsCategory::Engine:
        return "engine install directory scan verify updates launcher";
    case SettingsCategory::Storage:
        return "storage cache thumbnail clear size disk";
    case SettingsCategory::FileAssociations:
        return "file associations weproj weproject register";
    case SettingsCategory::About:
        return "about version logs installation documentation reset";
    default:
        return "";
    }
}

Color ParseHexColor(const std::string& hex) {
    unsigned int r = 91, g = 141, b = 239;
    if (hex.size() >= 7 && hex[0] == '#') {
        unsigned int value = 0;
        if (std::sscanf(hex.c_str() + 1, "%06x", &value) == 1) {
            r = (value >> 16) & 0xFF;
            g = (value >> 8) & 0xFF;
            b = value & 0xFF;
        }
    }
    return Color{
        static_cast<float>(r) / 255.0f,
        static_cast<float>(g) / 255.0f,
        static_cast<float>(b) / 255.0f,
        1.0f
    };
}

int IndexOfOption(const std::vector<std::string>& options, const std::string& value) {
    for (int i = 0; i < static_cast<int>(options.size()); ++i) {
        if (options[static_cast<std::size_t>(i)] == value) {
            return i;
        }
    }
    return 0;
}

// ── SettingsGroup ────────────────────────────────────────────────────────────

SettingsGroup::SettingsGroup(std::string title, std::string description)
    : m_Title(std::move(title))
    , m_Description(std::move(description)) {
}

void SettingsGroup::SetContent(const std::shared_ptr<Widget>& content) {
    ClearChildren();
    m_Content = content;
    if (m_Content) {
        AddChild(m_Content);
    }
}

Size SettingsGroup::Measure(const Size& availableSize) {
    const float s = LScale();
    const float padX = 16.0f * s;
    const float padY = 16.0f * s;
    const float titleSize = LMetric(MetricToken::TextSizeHeader) * s;
    const float descSize = LMetric(MetricToken::TextSizeCaption) * s;
    float headerH = titleSize + 4.0f * s;
    if (!m_Description.empty()) {
        headerH += descSize + 4.0f * s;
    }
    headerH += 12.0f * s;

    float contentH = 0.0f;
    if (m_Content) {
        const Size child = m_Content->Measure(Size{
            std::max(40.0f, availableSize.width - padX * 2.0f),
            availableSize.height
        });
        contentH = child.height;
    }

    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 480.0f * s,
        padY + headerH + contentH + padY
    };
    return m_DesiredSize;
}

void SettingsGroup::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float s = LScale();
    const float padX = 16.0f * s;
    const float padY = 16.0f * s;
    const float titleSize = LMetric(MetricToken::TextSizeHeader) * s;
    const float descSize = LMetric(MetricToken::TextSizeCaption) * s;
    float y = allottedRect.y + padY + titleSize + 4.0f * s;
    if (!m_Description.empty()) {
        y += descSize + 4.0f * s;
    }
    y += 12.0f * s;
    if (m_Content) {
        m_Content->Arrange(Rect{
            allottedRect.x + padX,
            y,
            allottedRect.width - padX * 2.0f,
            std::max(0.0f, allottedRect.y + allottedRect.height - padY - y)
        });
    }
}

void SettingsGroup::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = 8.0f * s;
    const float padX = 16.0f * s;
    const float padY = 16.0f * s;

    Color cardBg = LColor(ColorToken::PanelBackground);
    context.DrawRoundedRect(m_Geometry, cardBg, radius);

    if (m_Highlighted) {
        context.DrawRect(
            Rect{ m_Geometry.x + 4.0f * s, m_Geometry.y + 10.0f * s, 3.0f * s, m_Geometry.height - 20.0f * s },
            LColor(ColorToken::AccentPrimary));
    }

    const float titleSize = LMetric(MetricToken::TextSizeHeader) * s;
    const float descSize = LMetric(MetricToken::TextSizeCaption) * s;
    context.DrawText(
        m_Title,
        Point{ m_Geometry.x + padX, m_Geometry.y + padY },
        LColor(ColorToken::TextPrimary),
        titleSize,
        true);
    if (!m_Description.empty()) {
        context.DrawText(
            m_Description,
            Point{ m_Geometry.x + padX, m_Geometry.y + padY + titleSize + 4.0f * s },
            LColor(ColorToken::TextMuted),
            descSize);
    }

    if (m_Content && m_Content->IsVisible()) {
        m_Content->Paint(context);
    }
}

void SettingsGroup::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}


SettingsRow::SettingsRow(
    std::string label,
    std::string hint,
    std::shared_ptr<Widget> control,
    std::string searchText)
    : m_Label(std::move(label))
    , m_Hint(std::move(hint))
    , m_SearchText(searchText.empty() ? (m_Label + " " + m_Hint) : std::move(searchText))
    , m_Control(std::move(control)) {
}

void SettingsRow::AttachControl() {
    if (m_Control && !m_Control->GetParent()) {
        AddChild(m_Control);
    }
}

bool SettingsRow::MatchesQuery(const std::string& queryLower) const {
    return ContainsInsensitive(m_SearchText, queryLower);
}

Size SettingsRow::Measure(const Size& availableSize) {
    const float s = LScale();
    float controlH = 28.0f * s;
    float controlW = 160.0f * s;
    if (m_Control) {
        const Size cs = m_Control->Measure(Size{ availableSize.width * 0.5f, 40.0f * s });
        controlH = std::max(controlH, cs.height);
        controlW = cs.width;
    }
    const float labelBlock = m_Hint.empty() ? 18.0f * s : 32.0f * s;
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 440.0f * s,
        std::max(kRowH * s, std::max(labelBlock, controlH) + 8.0f * s)
    };
    (void)controlW;
    return m_DesiredSize;
}

void SettingsRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (!m_Control) {
        return;
    }
    const float s = LScale();
    const Size cs = m_Control->Measure(Size{ allottedRect.width * 0.55f, allottedRect.height });
    const float controlW = std::min(cs.width, allottedRect.width * 0.58f);
    m_Control->Arrange(Rect{
        allottedRect.x + allottedRect.width - controlW,
        allottedRect.y + (allottedRect.height - cs.height) * 0.5f,
        controlW,
        cs.height
    });
}

void SettingsRow::Paint(PaintContext& context) {
    const float s = LScale();
    if (m_Highlighted) {
        context.DrawRoundedRect(
            m_Geometry,
            Color::Lerp(Color::Transparent(), LColor(ColorToken::SelectedBackground), 0.55f),
            LMetric(MetricToken::CornerRadiusSmall) * s);
    }

    const float titleSize = LMetric(MetricToken::TextSizeBody) * s;
    const float hintSize = LMetric(MetricToken::TextSizeCaption) * s;
    float y = m_Geometry.y + (m_Hint.empty()
        ? (m_Geometry.height - titleSize) * 0.5f
        : 6.0f * s);
    context.DrawText(m_Label, Point{ m_Geometry.x, y }, LColor(ColorToken::TextPrimary), titleSize);
    if (!m_Hint.empty()) {
        context.DrawText(
            m_Hint,
            Point{ m_Geometry.x, y + titleSize + 2.0f * s },
            LColor(ColorToken::TextMuted),
            hintSize);
    }

    if (m_Control && m_Control->IsVisible()) {
        m_Control->Paint(context);
    }
}

void SettingsRow::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

// â”€â”€ ToggleSwitch â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

// â”€â”€ SettingsCheckBox â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

// â”€â”€ SettingsDropdown â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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
    const float textSize = 13.0f * s;
    for (const auto& opt : m_Options) {
        maxW = std::max(maxW, ApproxTextWidth(opt, textSize) + 36.0f * s);
    }
    if (availableSize.width > maxW) {
        maxW = availableSize.width;
    }
    float h = 34.0f * s;
    if (m_Open) {
        h += 4.0f * s + static_cast<float>(m_Options.size()) * 28.0f * s;
    }
    m_DesiredSize = Size{ maxW, h };
    return m_DesiredSize;
}

void SettingsDropdown::Arrange(const Rect& allottedRect) {
    const float s = LScale();
    const float triggerH = 34.0f * s;
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
    const float triggerH = 34.0f * s;
    return Rect{
        m_Geometry.x,
        m_Geometry.y + triggerH + 2.0f * s,
        m_Geometry.width,
        static_cast<float>(m_Options.size()) * 28.0f * s
    };
}

Rect SettingsDropdown::OptionRect(int index) const {
    const float s = LScale();
    const Rect menu = MenuRect();
    return Rect{ menu.x, menu.y + static_cast<float>(index) * 28.0f * s, menu.width, 28.0f * s };
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
    const float triggerH = 34.0f * s;
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

    const float textSize = 13.0f * s;
    context.DrawText(
        SelectedLabel(),
        Point{ trigger.x + 10.0f * s, trigger.y + (triggerH - textSize) * 0.5f },
        LColor(ColorToken::TextPrimary),
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
                i == m_Selected ? LColor(ColorToken::AccentPrimary) : LColor(ColorToken::TextPrimary),
                textSize);
        }
    }
}

void SettingsDropdown::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const float s = LScale();
    Rect trigger{ m_Geometry.x, m_Geometry.y, m_Geometry.width, 34.0f * s };
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

// â”€â”€ SettingsSegmented â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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
    m_DesiredSize = Size{ std::max(120.0f * s, w), 28.0f * s };
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

// â”€â”€ SettingsTextField â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

SettingsTextField::SettingsTextField(std::string text, float preferredWidth)
    : m_Text(std::move(text))
    , m_PreferredWidth(preferredWidth) {
}

void SettingsTextField::SetText(std::string text) {
    m_Text = std::move(text);
    InvalidateUI();
}

Size SettingsTextField::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    m_DesiredSize = Size{ m_PreferredWidth * s, 28.0f * s };
    return m_DesiredSize;
}

void SettingsTextField::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x + allottedRect.width - m_DesiredSize.width,
        allottedRect.y + (allottedRect.height - m_DesiredSize.height) * 0.5f,
        m_DesiredSize.width,
        m_DesiredSize.height
    };
}

void SettingsTextField::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;
    Color bg = LColor(ColorToken::InputBackground);
    if (m_HoverAnim > 0.01f) {
        bg = Color::Lerp(bg, LColor(ColorToken::HoverBackground), m_HoverAnim * 0.5f);
    }
    context.DrawRoundedRect(m_Geometry, bg, radius);
    context.DrawRoundedRectOutline(
        m_Geometry,
        m_FocusAnim > 0.5f ? LColor(ColorToken::BorderFocus) : LColor(ColorToken::BorderDefault),
        1.0f,
        radius);

    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    const float pad = 8.0f * s;
    const std::string& draw = m_Text.empty() ? m_Placeholder : m_Text;
    Color textColor = m_Text.empty() ? LColor(ColorToken::TextDisabled) : LColor(ColorToken::TextPrimary);
    std::string clipped = draw;
    const float maxW = m_Geometry.width - pad * 2.0f;
    while (clipped.size() > 3 && ApproxTextWidth(clipped, textSize) > maxW) {
        clipped.pop_back();
    }
    if (clipped.size() < draw.size() && clipped.size() > 3) {
        clipped = clipped.substr(0, clipped.size() - 3) + "...";
    }
    context.DrawText(
        clipped,
        Point{ m_Geometry.x + pad, m_Geometry.y + (m_Geometry.height - textSize) * 0.5f },
        textColor,
        textSize);

    if (m_Focused && m_CaretBlink < 0.5f) {
        const float cx = m_Geometry.x + pad + ApproxTextWidth(m_Text, textSize) + 1.0f;
        context.DrawLine(
            Point{ cx, m_Geometry.y + 6.0f * s },
            Point{ cx, m_Geometry.y + m_Geometry.height - 6.0f * s },
            LColor(ColorToken::AccentPrimary),
            1.5f);
    }
}

void SettingsTextField::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Geometry.Contains(event.position)) {
        m_Focused = true;
        InvalidateUI();
    }
}

void SettingsTextField::OnKeyDown(const KeyEvent& event) {
    if (!m_Focused) {
        return;
    }
    bool changed = false;
    if (event.key == we::platform::KeyCode::Backspace) {
        if (!m_Text.empty()) {
            m_Text.pop_back();
            changed = true;
        }
    } else if (event.key == we::platform::KeyCode::Enter || event.key == we::platform::KeyCode::Escape) {
        m_Focused = false;
    } else if (event.key == we::platform::KeyCode::Space) {
        m_Text.push_back(' ');
        changed = true;
    } else {
        const char typed = KeyCodeToChar(event.key, event.shiftDown);
        if (typed != 0 && m_Text.size() < 260) {
            m_Text.push_back(typed);
            changed = true;
        }
    }
    if (changed && m_OnChanged) {
        m_OnChanged(m_Text);
    }
    InvalidateUI();
}

void SettingsTextField::OnFocus() {
    m_Focused = true;
}

void SettingsTextField::OnBlur() {
    m_Focused = false;
}

bool SettingsTextField::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void SettingsTextField::Tick(float deltaTime) {
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered ? 1.0f : 0.0f,
        LMetric(MetricToken::HoverAnimationDamping));
    m_FocusAnim = Animator::Damp(m_FocusAnim, m_Focused ? 1.0f : 0.0f, 18.0f);
    m_CaretBlink += deltaTime;
    if (m_CaretBlink > 1.0f) {
        m_CaretBlink = 0.0f;
    }
    if (m_Focused) {
        InvalidateUI();
    }
    Widget::Tick(deltaTime);
}

// â”€â”€ PathPickerField â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

PathPickerField::PathPickerField(std::string path, bool folderMode)
    : m_Path(std::move(path))
    , m_FolderMode(folderMode) {
}

void PathPickerField::SetPath(std::string path) {
    m_Path = std::move(path);
    InvalidateUI();
}

Size PathPickerField::Measure(const Size& availableSize) {
    const float s = LScale();
    const float w = availableSize.width > 80.0f * s ? availableSize.width : 320.0f * s;
    m_DesiredSize = Size{ w, 34.0f * s };
    return m_DesiredSize;
}

void PathPickerField::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - m_DesiredSize.height) * 0.5f,
        allottedRect.width,
        m_DesiredSize.height
    };
}

Rect PathPickerField::BrowseRect() const {
    const float s = LScale();
    const float w = 72.0f * s;
    return Rect{
        m_Geometry.x + m_Geometry.width - w,
        m_Geometry.y,
        w,
        m_Geometry.height
    };
}

void PathPickerField::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;
    const Rect browse = BrowseRect();
    Rect field{
        m_Geometry.x,
        m_Geometry.y,
        m_Geometry.width - browse.width - 6.0f * s,
        m_Geometry.height
    };

    context.DrawRoundedRect(field, LColor(ColorToken::InputBackground), radius);
    context.DrawRoundedRectOutline(field, LColor(ColorToken::BorderDefault), 1.0f, radius);

    Color browseBg = LColor(ColorToken::ButtonPrimaryBackground);
    if (m_HoverBrowse) {
        browseBg = Color::Lerp(browseBg, LColor(ColorToken::ButtonPrimaryHover), 0.65f);
    }
    context.DrawRoundedRect(browse, browseBg, radius);

    const float textSize = LMetric(MetricToken::TextSizeCaption) * s;
    const std::string shown = EllipsizePath(m_Path.empty() ? "No path selected" : m_Path, 42);
    context.DrawText(
        shown,
        Point{ field.x + 8.0f * s, field.y + (field.height - textSize) * 0.5f },
        m_Path.empty() ? LColor(ColorToken::TextDisabled) : LColor(ColorToken::TextPrimary),
        textSize);

    const float browseLabelSize = LMetric(MetricToken::TextSizeCaption) * s;
    const std::string browseLabel = "Browse";
    const float bw = ApproxTextWidth(browseLabel, browseLabelSize);
    context.DrawText(
        browseLabel,
        Point{ browse.x + (browse.width - bw) * 0.5f, browse.y + (browse.height - browseLabelSize) * 0.5f },
        LColor(ColorToken::TextPrimary),
        browseLabelSize,
        true);
}

void PathPickerField::OpenPicker() {
    we::platform::FileDialogDesc desc{};
    desc.mode = m_FolderMode ? we::platform::FileDialogMode::OpenFolder : we::platform::FileDialogMode::OpenFile;
    desc.title = m_DialogTitle.c_str();
    desc.defaultPath = m_Path.c_str();
    if (!m_FolderMode) {
        desc.filters = { { "All Files", "*.*" } };
    }
    const auto paths = we::platform::Platform::Get().ShowFileDialog(desc);
    if (!paths.empty()) {
        m_Path = paths.front();
        if (m_OnChanged) {
            m_OnChanged(m_Path);
        }
        InvalidateUI();
    }
}

void PathPickerField::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && BrowseRect().Contains(event.position)) {
        m_PressedBrowse = true;
    }
}

void PathPickerField::OnMouseMove(const MouseEvent& event) {
    m_HoverBrowse = BrowseRect().Contains(event.position);
    m_Hovered = m_Geometry.Contains(event.position);
}

void PathPickerField::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_PressedBrowse && BrowseRect().Contains(event.position)) {
        OpenPicker();
    }
    m_PressedBrowse = false;
}

bool PathPickerField::ShowsPointerCursor(const Point& position) const {
    return BrowseRect().Contains(position);
}

void PathPickerField::Tick(float deltaTime) {
    (void)deltaTime;
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_HoverBrowse ? 1.0f : 0.0f,
        LMetric(MetricToken::HoverAnimationDamping));
    Widget::Tick(deltaTime);
}

// â”€â”€ ColorSwatchPicker â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

ColorSwatchPicker::ColorSwatchPicker(std::string hexColor)
    : m_Hex(std::move(hexColor)) {
    m_HoverAnims.assign(sizeof(kAccentPalette) / sizeof(kAccentPalette[0]), 0.0f);
    m_Selected = 0;
    for (int i = 0; i < static_cast<int>(sizeof(kAccentPalette) / sizeof(kAccentPalette[0])); ++i) {
        if (m_Hex == kAccentPalette[i]) {
            m_Selected = i;
            break;
        }
    }
}

void ColorSwatchPicker::SetColorHex(std::string hex) {
    m_Hex = std::move(hex);
    for (int i = 0; i < static_cast<int>(sizeof(kAccentPalette) / sizeof(kAccentPalette[0])); ++i) {
        if (m_Hex == kAccentPalette[i]) {
            m_Selected = i;
            break;
        }
    }
    InvalidateUI();
}

Size ColorSwatchPicker::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    const int count = static_cast<int>(sizeof(kAccentPalette) / sizeof(kAccentPalette[0]));
    m_DesiredSize = Size{ static_cast<float>(count) * 26.0f * s, 24.0f * s };
    return m_DesiredSize;
}

void ColorSwatchPicker::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x + allottedRect.width - m_DesiredSize.width,
        allottedRect.y + (allottedRect.height - m_DesiredSize.height) * 0.5f,
        m_DesiredSize.width,
        m_DesiredSize.height
    };
}

Rect ColorSwatchPicker::SwatchRect(int index) const {
    const float s = LScale();
    return Rect{
        m_Geometry.x + static_cast<float>(index) * 26.0f * s,
        m_Geometry.y,
        22.0f * s,
        22.0f * s
    };
}

int ColorSwatchPicker::HitSwatch(const Point& p) const {
    const int count = static_cast<int>(sizeof(kAccentPalette) / sizeof(kAccentPalette[0]));
    for (int i = 0; i < count; ++i) {
        if (SwatchRect(i).Contains(p)) {
            return i;
        }
    }
    return -1;
}

void ColorSwatchPicker::Paint(PaintContext& context) {
    const float s = LScale();
    const int count = static_cast<int>(sizeof(kAccentPalette) / sizeof(kAccentPalette[0]));
    for (int i = 0; i < count; ++i) {
        const Rect r = SwatchRect(i);
        context.DrawRoundedRect(r, ParseHexColor(kAccentPalette[i]), 6.0f * s);
        if (i == m_Selected) {
            context.DrawRoundedRectOutline(r, LColor(ColorToken::TextPrimary), 2.0f, 6.0f * s);
        } else if (m_HoverAnims[static_cast<std::size_t>(i)] > 0.01f) {
            context.DrawRoundedRectOutline(r, LColor(ColorToken::BorderFocus), 1.0f, 6.0f * s);
        }
    }
}

void ColorSwatchPicker::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const int hit = HitSwatch(event.position);
    if (hit >= 0) {
        m_Selected = hit;
        m_Hex = kAccentPalette[hit];
        if (m_OnChanged) {
            m_OnChanged(m_Hex);
        }
        InvalidateUI();
    }
}

void ColorSwatchPicker::OnMouseMove(const MouseEvent& event) {
    m_Hovered = HitSwatch(event.position);
}

bool ColorSwatchPicker::ShowsPointerCursor(const Point& position) const {
    return HitSwatch(position) >= 0;
}

void ColorSwatchPicker::Tick(float deltaTime) {
    (void)deltaTime;
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    for (int i = 0; i < static_cast<int>(m_HoverAnims.size()); ++i) {
        m_HoverAnims[static_cast<std::size_t>(i)] = Animator::Damp(
            m_HoverAnims[static_cast<std::size_t>(i)],
            i == m_Hovered ? 1.0f : 0.0f,
            damp);
    }
    Widget::Tick(deltaTime);
}

// â”€â”€ NumberStepper â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

NumberStepper::NumberStepper(float value, float minValue, float maxValue, float step, std::string suffix)
    : m_Value(value)
    , m_Min(minValue)
    , m_Max(maxValue)
    , m_Step(step)
    , m_Suffix(std::move(suffix)) {
}

void NumberStepper::SetValue(float value) {
    m_Value = std::clamp(value, m_Min, m_Max);
    InvalidateUI();
}

Size NumberStepper::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    m_DesiredSize = Size{ 110.0f * s, 28.0f * s };
    return m_DesiredSize;
}

void NumberStepper::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x + allottedRect.width - m_DesiredSize.width,
        allottedRect.y + (allottedRect.height - m_DesiredSize.height) * 0.5f,
        m_DesiredSize.width,
        m_DesiredSize.height
    };
}

Rect NumberStepper::MinusRect() const {
    const float s = LScale();
    return Rect{ m_Geometry.x, m_Geometry.y, 28.0f * s, m_Geometry.height };
}

Rect NumberStepper::PlusRect() const {
    const float s = LScale();
    return Rect{ m_Geometry.x + m_Geometry.width - 28.0f * s, m_Geometry.y, 28.0f * s, m_Geometry.height };
}

NumberStepper::Zone NumberStepper::HitTest(const Point& p) const {
    if (MinusRect().Contains(p)) {
        return Zone::Minus;
    }
    if (PlusRect().Contains(p)) {
        return Zone::Plus;
    }
    return Zone::None;
}

void NumberStepper::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;
    context.DrawRoundedRect(m_Geometry, LColor(ColorToken::InputBackground), radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderDefault), 1.0f, radius);

    auto paintBtn = [&](const Rect& r, const char* icon, bool hot) {
        if (hot) {
            context.DrawRoundedRect(r, LColor(ColorToken::HoverBackground), radius);
        }
        IconPainter::DrawIcon(
            context,
            icon,
            Rect{ r.x + (r.width - 12.0f * s) * 0.5f, r.y + (r.height - 12.0f * s) * 0.5f, 12.0f * s, 12.0f * s },
            LColor(ColorToken::IconSecondary));
    };
    paintBtn(MinusRect(), Icons::MinusName, m_Hover == Zone::Minus);
    paintBtn(PlusRect(), Icons::PlusName, m_Hover == Zone::Plus);

    char buf[48];
    if (m_Suffix == "%") {
        std::snprintf(buf, sizeof(buf), "%.0f%%", m_Value);
    } else if (!m_Suffix.empty()) {
        std::snprintf(buf, sizeof(buf), "%.0f %s", m_Value, m_Suffix.c_str());
    } else {
        std::snprintf(buf, sizeof(buf), "%.1f", m_Value);
    }
    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    const std::string label = buf;
    const float tw = ApproxTextWidth(label, textSize);
    context.DrawText(
        label,
        Point{ m_Geometry.x + (m_Geometry.width - tw) * 0.5f, m_Geometry.y + (m_Geometry.height - textSize) * 0.5f },
        LColor(ColorToken::TextPrimary),
        textSize);
}

void NumberStepper::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = HitTest(event.position);
    }
}

void NumberStepper::OnMouseMove(const MouseEvent& event) {
    m_Hover = HitTest(event.position);
}

void NumberStepper::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const Zone hit = HitTest(event.position);
    if (hit == m_Pressed && hit != Zone::None) {
        const float next = hit == Zone::Plus ? m_Value + m_Step : m_Value - m_Step;
        SetValue(next);
        if (m_OnChanged) {
            m_OnChanged(m_Value);
        }
    }
    m_Pressed = Zone::None;
}

bool NumberStepper::ShowsPointerCursor(const Point& position) const {
    return HitTest(position) != Zone::None;
}

void NumberStepper::Tick(float deltaTime) {
    (void)deltaTime;
    Widget::Tick(deltaTime);
}

// â”€â”€ CacheUsageBar â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

CacheUsageBar::CacheUsageBar(std::string label, float usedMb, float capacityMb)
    : m_Label(std::move(label))
    , m_UsedMb(usedMb)
    , m_CapacityMb(std::max(1.0f, capacityMb)) {
}

void CacheUsageBar::SetUsage(float usedMb, float capacityMb) {
    m_UsedMb = usedMb;
    m_CapacityMb = std::max(1.0f, capacityMb);
    InvalidateUI();
}

Size CacheUsageBar::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 400.0f * s,
        36.0f * s
    };
    return m_DesiredSize;
}

void CacheUsageBar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void CacheUsageBar::Paint(PaintContext& context) {
    const float s = LScale();
    const float textSize = LMetric(MetricToken::TextSizeCaption) * s;
    context.DrawText(m_Label, Point{ m_Geometry.x, m_Geometry.y }, LColor(ColorToken::TextSecondary), textSize);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.0f / %.0f MB", m_UsedMb, m_CapacityMb);
    const std::string usage = buf;
    const float uw = ApproxTextWidth(usage, textSize);
    context.DrawText(
        usage,
        Point{ m_Geometry.x + m_Geometry.width - uw, m_Geometry.y },
        LColor(ColorToken::TextMuted),
        textSize);

    Rect track{
        m_Geometry.x,
        m_Geometry.y + 18.0f * s,
        m_Geometry.width,
        8.0f * s
    };
    context.DrawRoundedRect(track, LColor(ColorToken::InputBackground), 4.0f * s);
    const float ratio = std::clamp(m_UsedMb / m_CapacityMb, 0.0f, 1.0f);
    if (ratio > 0.001f) {
        context.DrawRoundedRect(
            Rect{ track.x, track.y, track.width * ratio, track.height },
            LColor(ColorToken::AccentPrimary),
            4.0f * s);
    }
}

// â”€â”€ AppearancePreviewPanel â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void AppearancePreviewPanel::SetTheme(std::string theme) {
    m_Theme = std::move(theme);
    InvalidateUI();
}

void AppearancePreviewPanel::SetAccentHex(std::string hex) {
    m_AccentHex = std::move(hex);
    InvalidateUI();
}

void AppearancePreviewPanel::SetIconStyle(std::string style) {
    m_IconStyle = std::move(style);
    InvalidateUI();
}

void AppearancePreviewPanel::SetUiScale(float scale) {
    m_UiScale = scale;
    InvalidateUI();
}

void AppearancePreviewPanel::SetFontSize(float size) {
    m_FontSize = size;
    InvalidateUI();
}

Size AppearancePreviewPanel::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 420.0f * s,
        148.0f * s
    };
    return m_DesiredSize;
}

void AppearancePreviewPanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void AppearancePreviewPanel::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusMedium) * s;
    context.DrawRoundedRect(m_Geometry, LColor(ColorToken::PanelContentBackground), radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderDefault), 1.0f, radius);

    const Color accent = ParseHexColor(m_AccentHex);
    Rect chrome{ m_Geometry.x + 12.0f * s, m_Geometry.y + 12.0f * s, m_Geometry.width - 24.0f * s, 28.0f * s };
    context.DrawRoundedRect(chrome, LColor(ColorToken::HeaderBackground), 6.0f * s);
    context.DrawRoundedRect(
        Rect{ chrome.x + 8.0f * s, chrome.y + 8.0f * s, 12.0f * s, 12.0f * s },
        accent,
        6.0f * s);

    const float titleSize = (m_FontSize * 0.85f) * m_UiScale * s;
    context.DrawText(
        m_Theme + " preview",
        Point{ chrome.x + 28.0f * s, chrome.y + (chrome.height - titleSize) * 0.5f },
        LColor(ColorToken::TextPrimary),
        titleSize,
        true);

    Rect card{
        m_Geometry.x + 12.0f * s,
        chrome.y + chrome.height + 10.0f * s,
        m_Geometry.width - 24.0f * s,
        m_Geometry.height - (chrome.y - m_Geometry.y) - chrome.height - 22.0f * s
    };
    context.DrawRoundedRect(card, LColor(ColorToken::PanelBackground), 6.0f * s);

    const float bodySize = (m_FontSize * 0.75f) * m_UiScale * s;
    context.DrawText(
        "Accent Â· " + m_AccentHex + "   Icons Â· " + m_IconStyle,
        Point{ card.x + 12.0f * s, card.y + 12.0f * s },
        LColor(ColorToken::TextSecondary),
        bodySize);
    context.DrawText(
        "UI Scale " + std::to_string(static_cast<int>(m_UiScale * 100.0f + 0.5f)) + "%   Font "
            + std::to_string(static_cast<int>(m_FontSize + 0.5f)) + " px",
        Point{ card.x + 12.0f * s, card.y + 12.0f * s + bodySize + 6.0f * s },
        LColor(ColorToken::TextMuted),
        bodySize);

    const float icon = 16.0f * s * m_UiScale;
    IconPainter::DrawIcon(
        context,
        Icons::Cube3DName,
        Rect{ card.x + card.width - icon - 14.0f * s, card.y + 14.0f * s, icon, icon },
        accent);
    IconPainter::DrawIcon(
        context,
        Icons::SettingsName,
        Rect{ card.x + card.width - icon * 2.0f - 24.0f * s, card.y + 14.0f * s, icon, icon },
        LColor(ColorToken::IconSecondary));
}

void AppearancePreviewPanel::Tick(float deltaTime) {
    m_Pulse += deltaTime;
    Widget::Tick(deltaTime);
}

// â”€â”€ SettingsActionBar â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void SettingsActionBar::AddAction(
    std::string label,
    const char* icon,
    std::function<void()> onClick,
    bool primary) {
    std::shared_ptr<Widget> btn;
    if (primary) {
        auto p = std::make_shared<PrimaryButton>(std::move(label), icon);
        p->SetOnClicked(std::move(onClick));
        btn = p;
    } else {
        auto s = std::make_shared<SecondaryButton>(std::move(label), icon);
        s->SetOnClicked(std::move(onClick));
        btn = s;
    }
    m_Buttons.push_back(btn);
    AddChild(btn);
}

Size SettingsActionBar::Measure(const Size& availableSize) {
    const float s = LScale();
    float w = 0.0f;
    float h = 28.0f * s;
    for (auto& btn : m_Buttons) {
        if (!btn) {
            continue;
        }
        const Size cs = btn->Measure(Size{ 200.0f * s, 36.0f * s });
        w += cs.width + 8.0f * s;
        h = std::max(h, cs.height);
    }
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : std::max(w, 200.0f * s),
        h
    };
    return m_DesiredSize;
}

void SettingsActionBar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float s = LScale();
    float x = allottedRect.x;
    for (auto& btn : m_Buttons) {
        if (!btn) {
            continue;
        }
        const Size cs = btn->Measure(Size{ 200.0f * s, allottedRect.height });
        btn->Arrange(Rect{ x, allottedRect.y, cs.width, cs.height });
        x += cs.width + 8.0f * s;
    }
}

void SettingsActionBar::Paint(PaintContext& context) {
    for (auto& btn : m_Buttons) {
        if (btn && btn->IsVisible()) {
            btn->Paint(context);
        }
    }
}

void SettingsActionBar::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

} // namespace we::programs::welauncher
