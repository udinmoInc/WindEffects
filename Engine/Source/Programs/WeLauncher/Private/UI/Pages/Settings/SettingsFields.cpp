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
    m_DesiredSize = Size{ m_PreferredWidth * s, LControlH(ControlSize::Compact) * s };
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
    Color textColor = m_Text.empty() ? LColor(ColorToken::TextDisabled) : InputValueTextColor();
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

// Ã¢â€â‚¬Ã¢â€â‚¬ PathPickerField Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

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
    m_DesiredSize = Size{ w, LControlH() * s };
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

    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    const std::string shown = EllipsizePath(m_Path.empty() ? "No path selected" : m_Path, 42);
    context.DrawText(
        shown,
        Point{ field.x + 8.0f * s, field.y + (field.height - textSize) * 0.5f },
        m_Path.empty() ? LColor(ColorToken::TextDisabled) : InputValueTextColor(),
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

// Ã¢â€â‚¬Ã¢â€â‚¬ ColorSwatchPicker Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

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

// Ã¢â€â‚¬Ã¢â€â‚¬ NumberStepper Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

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
    m_DesiredSize = Size{ 110.0f * s, LControlH(ControlSize::Compact) * s };
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
    const float btn = LControlH(ControlSize::Compact) * s;
    return Rect{ m_Geometry.x, m_Geometry.y, btn, m_Geometry.height };
}

Rect NumberStepper::PlusRect() const {
    const float s = LScale();
    const float btn = LControlH(ControlSize::Compact) * s;
    return Rect{ m_Geometry.x + m_Geometry.width - btn, m_Geometry.y, btn, m_Geometry.height };
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

// Ã¢â€â‚¬Ã¢â€â‚¬ CacheUsageBar Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

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

// Ã¢â€â‚¬Ã¢â€â‚¬ AppearancePreviewPanel Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

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
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderSubtle), 1.0f, radius);

    const Color accent = ParseHexColor(m_AccentHex);
    Rect chrome{
        m_Geometry.x + LMetric(MetricToken::CardPadding) * s,
        m_Geometry.y + LMetric(MetricToken::CardPadding) * s,
        m_Geometry.width - LMetric(MetricToken::CardPadding) * 2.0f * s,
        LControlH(ControlSize::Compact) * s
    };
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
    context.DrawRoundedRect(card, LColor(ColorToken::CardBackground), 6.0f * s);
    context.DrawRoundedRectOutline(card, LColor(ColorToken::BorderSubtle), 1.0f, 6.0f * s);

    const float bodySize = (m_FontSize * 0.75f) * m_UiScale * s;
    context.DrawText(
        "Accent Ã‚Â· " + m_AccentHex + "   Icons Ã‚Â· " + m_IconStyle,
        Point{ card.x + 12.0f * s, card.y + 12.0f * s },
        LColor(ColorToken::TextSecondary),
        bodySize);
    context.DrawText(
        "UI Scale " + std::to_string(static_cast<int>(m_UiScale * 100.0f + 0.5f)) + "%   Font "
            + std::to_string(static_cast<int>(m_FontSize + 0.5f)) + " px",
        Point{ card.x + 12.0f * s, card.y + 12.0f * s + bodySize + 6.0f * s },
        LColor(ColorToken::TextHint),
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

// Ã¢â€â‚¬Ã¢â€â‚¬ SettingsActionBar Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

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
    float h = LControlH(ControlSize::Compact) * s;
    for (auto& btn : m_Buttons) {
        if (!btn) {
            continue;
        }
        const Size cs = btn->Measure(Size{ 200.0f * s, LControlH(ControlSize::Large) * s });
        w += cs.width + LMetric(MetricToken::ButtonSpacing) * 2.0f * s;
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
        x += cs.width + LMetric(MetricToken::ButtonSpacing) * 2.0f * s;
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
