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
    const TypographySpec titleSpec = ResolveTypography(TypographyToken::SectionTitle);
    const TypographySpec descSpec = ResolveTypography(TypographyToken::Hint);
    float headerH = titleSpec.lineHeightPx * s;
    if (!m_Description.empty()) {
        headerH += descSpec.lineHeightPx * s + LMetric(MetricToken::LabelHintGap) * s;
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
    const TypographySpec titleSpec = ResolveTypography(TypographyToken::SectionTitle);
    const TypographySpec descSpec = ResolveTypography(TypographyToken::Hint);
    float y = allottedRect.y + padY + titleSpec.lineHeightPx * s;
    if (!m_Description.empty()) {
        y += descSpec.lineHeightPx * s + LMetric(MetricToken::LabelHintGap) * s;
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

    Color cardBg = LColor(ColorToken::CardBackground);
    context.DrawRoundedRect(m_Geometry, cardBg, radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderSubtle), 1.0f, radius);

    if (m_Highlighted) {
        context.DrawRect(
            Rect{ m_Geometry.x + 4.0f * s, m_Geometry.y + 10.0f * s, 3.0f * s, m_Geometry.height - 20.0f * s },
            LColor(ColorToken::AccentPrimary));
    }

    const TypographySpec titleSpec = ResolveTypography(TypographyToken::SectionTitle);
    const TypographySpec descSpec = ResolveTypography(TypographyToken::Hint);
    context.DrawText(
        m_Title,
        Point{ m_Geometry.x + padX, m_Geometry.y + padY },
        titleSpec.color,
        titleSpec.sizePx * s,
        static_cast<we::runtime::text::layout::FontWeight>(titleSpec.weight));
    if (!m_Description.empty()) {
        context.DrawText(
            m_Description,
            Point{
                m_Geometry.x + padX,
                m_Geometry.y + padY + titleSpec.lineHeightPx * s + LMetric(MetricToken::LabelHintGap) * s
            },
            descSpec.color,
            descSpec.sizePx * s,
            static_cast<we::runtime::text::layout::FontWeight>(descSpec.weight));
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
    float controlH = LControlH(ControlSize::Compact) * s;
    float controlW = 160.0f * s;
    if (m_Control) {
        const Size cs = m_Control->Measure(Size{ availableSize.width * 0.5f, LMetric(MetricToken::FormRowHeight) * s });
        controlH = std::max(controlH, cs.height);
        controlW = cs.width;
    }
    const TypographySpec labelSpec = ResolveTypography(TypographyToken::Label);
    const TypographySpec hintSpec = ResolveTypography(TypographyToken::Hint);
    const float labelBlock = m_Hint.empty()
        ? labelSpec.lineHeightPx * s
        : (labelSpec.lineHeightPx + hintSpec.lineHeightPx + LMetric(MetricToken::LabelHintGap)) * s;
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 440.0f * s,
        std::max(LMetric(MetricToken::FormRowHeight) * s, std::max(labelBlock, controlH) + LMetric(MetricToken::FormRowGap) * s)
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

    const TypographySpec labelSpec = ResolveTypography(TypographyToken::Label);
    const TypographySpec hintSpec = ResolveTypography(TypographyToken::Hint);
    const float labelSize = labelSpec.sizePx * s;
    const float hintSize = hintSpec.sizePx * s;
    const float labelHintGap = LMetric(MetricToken::LabelHintGap) * s;
    float y = m_Geometry.y + (m_Hint.empty()
        ? (m_Geometry.height - labelSpec.lineHeightPx * s) * 0.5f
        : 6.0f * s);
    context.DrawText(
        m_Label,
        Point{ m_Geometry.x, y },
        labelSpec.color,
        labelSize,
        static_cast<we::runtime::text::layout::FontWeight>(labelSpec.weight));
    if (!m_Hint.empty()) {
        context.DrawText(
            m_Hint,
            Point{ m_Geometry.x, y + labelSpec.lineHeightPx * s + labelHintGap },
            hintSpec.color,
            hintSize,
            static_cast<we::runtime::text::layout::FontWeight>(hintSpec.weight));
    }

    if (m_Control && m_Control->IsVisible()) {
        m_Control->Paint(context);
    }
}

void SettingsRow::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

// Ã¢â€â‚¬Ã¢â€â‚¬ ToggleSwitch Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

} // namespace we::programs::welauncher
