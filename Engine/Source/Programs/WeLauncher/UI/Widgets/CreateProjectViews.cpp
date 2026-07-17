#include "UI/Widgets/CreateProjectViews.h"

#include "UI/LauncherHelpers.h"
#include "UI/Widgets/ManagerViews.h"

#include "KindUI/Core/Animator.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace we::runtime::kindui;

namespace we::programs::welauncher {
namespace {

bool HasTag(const ProjectTemplateInfo& tmpl, const char* tag) {
    for (const auto& t : tmpl.tags) {
        if (t == tag) {
            return true;
        }
    }
    return false;
}

float ApproxW(const std::string& text, float textSize) {
    return static_cast<float>(text.size()) * textSize * 0.55f;
}

std::string Ellipsize(const std::string& text, float maxWidth, float textSize) {
    if (ApproxW(text, textSize) <= maxWidth) {
        return text;
    }
    std::string out = text;
    while (out.size() > 3 && ApproxW(out + "...", textSize) > maxWidth) {
        out.pop_back();
    }
    return out + "...";
}

} // namespace

bool TemplateMatchesWizardFilter(const ProjectTemplateInfo& tmpl, const std::string& filter) {
    if (filter.empty() || filter == "All") {
        return true;
    }
    if (filter == "Games") {
        return tmpl.category == "Games";
    }
    if (filter == "3D") {
        return HasTag(tmpl, "3D");
    }
    if (filter == "VR") {
        return HasTag(tmpl, "VR") || tmpl.id == "VR";
    }
    if (filter == "XR") {
        return tmpl.category == "XR" || HasTag(tmpl, "XR");
    }
    if (filter == "Samples") {
        return tmpl.category == "Core" || HasTag(tmpl, "Starter");
    }
    return tmpl.category == filter;
}

std::string EstimateTemplateDiskUsage(const ProjectTemplateInfo& tmpl) {
    std::uint64_t mb = 48;
    if (tmpl.id == "Empty") {
        mb = 12;
    } else if (tmpl.id == "Blank") {
        mb = 35;
    } else if (tmpl.id == "VR") {
        mb = 220;
    } else if (tmpl.id == "Vehicle") {
        mb = 180;
    } else if (tmpl.id == "FirstPerson" || tmpl.id == "ThirdPerson") {
        mb = 140;
    } else if (tmpl.id == "TopDown") {
        mb = 110;
    }
    mb += static_cast<std::uint64_t>(tmpl.starterAssets.size()) * 8;
    mb += static_cast<std::uint64_t>(tmpl.plugins.size()) * 12;
    if (mb >= 1024) {
        return std::to_string(mb / 1024) + "." + std::to_string((mb % 1024) / 100) + " GB";
    }
    return std::to_string(mb) + " MB";
}

std::string TemplateHardwareHint(const ProjectTemplateInfo& tmpl) {
    if (tmpl.id == "VR" || HasTag(tmpl, "XR")) {
        return "VR-ready GPU, 16 GB RAM";
    }
    if (tmpl.id == "Vehicle") {
        return "Dedicated GPU, 16 GB RAM";
    }
    if (tmpl.id == "Empty" || tmpl.id == "Blank") {
        return "Integrated GPU, 8 GB RAM";
    }
    return "Dedicated GPU recommended, 8 GB RAM";
}

// ── WizardDialogShell ────────────────────────────────────────────────────────

WizardDialogShell::WizardDialogShell() = default;

void WizardDialogShell::SetContent(const std::shared_ptr<Widget>& content) {
    ClearChildren();
    m_Content = content;
    if (m_Content) {
        AddChild(m_Content);
    }
}

Size WizardDialogShell::Measure(const Size& availableSize) {
    const float s = LScale();
    const float w = std::min(kWidth * s, availableSize.width > 0.0f ? availableSize.width : kWidth * s);
    const float h = std::min(kHeight * s, availableSize.height > 0.0f ? availableSize.height : kHeight * s);
    if (m_Content) {
        m_Content->Measure(Size{ w, h });
    }
    m_DesiredSize = Size{ w, h };
    return m_DesiredSize;
}

void WizardDialogShell::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_Content) {
        m_Content->Arrange(allottedRect);
    }
}

void WizardDialogShell::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = kRadius * s;

    Color shadow{ 0.0f, 0.0f, 0.0f, 0.55f };
    context.DrawShadow(
        Rect{
            m_Geometry.x,
            m_Geometry.y + 6.0f * s,
            m_Geometry.width,
            m_Geometry.height
        },
        shadow,
        radius,
        28.0f * s);

    context.DrawRoundedRect(m_Geometry, LColor(ColorToken::DialogBackground), radius);
    context.DrawRoundedRectOutline(
        m_Geometry,
        LColor(ColorToken::BorderDefault),
        1.0f * s,
        radius);

    if (m_Content && m_Content->IsVisible()) {
        m_Content->Paint(context);
    }
}

// ── FilterChip ───────────────────────────────────────────────────────────────

FilterChip::FilterChip(std::string label, bool selected)
    : m_Label(std::move(label))
    , m_Selected(selected) {
}

Size FilterChip::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    const float textSize = 13.0f * s;
    m_DesiredSize = Size{
        ApproxW(m_Label, textSize) + 24.0f * s,
        kHeight * s
    };
    return m_DesiredSize;
}

void FilterChip::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - m_DesiredSize.height) * 0.5f,
        m_DesiredSize.width,
        m_DesiredSize.height
    };
}

void FilterChip::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = 8.0f * s;
    Color bg = m_Selected
        ? LColor(ColorToken::SelectedBackground)
        : Color::Lerp(LColor(ColorToken::PanelBackground), LColor(ColorToken::HoverBackground), m_HoverAnim);
    context.DrawRoundedRect(m_Geometry, bg, radius);
    if (m_Selected) {
        context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::AccentPrimary), 1.0f * s, radius);
    }

    const float textSize = 13.0f * s;
    context.DrawText(
        m_Label,
        Point{
            m_Geometry.x + (m_Geometry.width - ApproxW(m_Label, textSize)) * 0.5f,
            m_Geometry.y + (m_Geometry.height - textSize) * 0.5f
        },
        m_Selected ? LColor(ColorToken::TextPrimary) : LColor(ColorToken::TextSecondary),
        textSize,
        m_Selected);
}

void FilterChip::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = m_Geometry.Contains(event.position);
    }
}

void FilterChip::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed && m_Geometry.Contains(event.position) && m_OnClicked) {
        m_OnClicked();
    }
    m_Pressed = false;
}

bool FilterChip::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void FilterChip::Tick(float deltaTime) {
    (void)deltaTime;
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    m_HoverAnim = Animator::Damp(m_HoverAnim, IsHovered() && !m_Selected ? 1.0f : 0.0f, damp);
    Widget::Tick(deltaTime);
}

// ── CreateTemplateRow ────────────────────────────────────────────────────────

CreateTemplateRow::CreateTemplateRow(ProjectTemplateInfo info, bool selected)
    : m_Info(std::move(info))
    , m_Selected(selected) {
}

Size CreateTemplateRow::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 420.0f * s,
        kRowH * s
    };
    return m_DesiredSize;
}

void CreateTemplateRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void CreateTemplateRow::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = 8.0f * s;

    Color bg = m_Selected
        ? LColor(ColorToken::SelectedBackground)
        : Color::Lerp(Color::Transparent(), LColor(ColorToken::HoverBackground), m_HoverAnim);
    if (m_Selected || m_HoverAnim > 0.01f) {
        context.DrawRoundedRect(m_Geometry, bg, radius);
    }
    if (m_Selected) {
        context.DrawRect(
            Rect{ m_Geometry.x, m_Geometry.y + 10.0f * s, 3.0f * s, m_Geometry.height - 20.0f * s },
            LColor(ColorToken::AccentPrimary));
    }

    const float pad = 12.0f * s;
    const float iconBox = 36.0f * s;
    context.DrawRoundedRect(
        Rect{
            m_Geometry.x + pad,
            m_Geometry.y + (m_Geometry.height - iconBox) * 0.5f,
            iconBox,
            iconBox
        },
        LColor(ColorToken::PanelBackground),
        8.0f * s);
    IconPainter::DrawIcon(
        context,
        TemplateTypeIcon(m_Info.id),
        Rect{
            m_Geometry.x + pad + 8.0f * s,
            m_Geometry.y + (m_Geometry.height - 20.0f * s) * 0.5f,
            20.0f * s,
            20.0f * s
        },
        LColor(ColorToken::AccentPrimary));

    const float textX = m_Geometry.x + pad + iconBox + 12.0f * s;
    const float textW = std::max(40.0f, m_Geometry.width - (textX - m_Geometry.x) - pad);
    const float titleSize = 13.0f * s;
    const float metaSize = 12.0f * s;

    context.DrawText(
        Ellipsize(m_Info.displayName, textW, titleSize),
        Point{ textX, m_Geometry.y + 12.0f * s },
        LColor(ColorToken::TextPrimary),
        titleSize,
        true);

    const std::string category = m_Info.category.empty() ? "3D" : m_Info.category;
    context.DrawText(
        category,
        Point{ textX, m_Geometry.y + 12.0f * s + titleSize + 2.0f * s },
        LColor(ColorToken::AccentPrimary),
        metaSize);

    const std::string desc = m_Info.description.empty() ? "No description" : m_Info.description;
    context.DrawText(
        Ellipsize(desc, textW, metaSize),
        Point{ textX, m_Geometry.y + m_Geometry.height - metaSize - 12.0f * s },
        LColor(ColorToken::TextMuted),
        metaSize);
}

void CreateTemplateRow::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = m_Geometry.Contains(event.position);
    }
}

void CreateTemplateRow::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left || !m_Pressed || !m_Geometry.Contains(event.position)) {
        m_Pressed = false;
        return;
    }
    m_Pressed = false;

    const auto now = std::chrono::steady_clock::now();
    const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastClick).count();
    m_LastClick = now;
    if (dt > 0 && dt < 350 && m_OnActivate) {
        m_OnActivate();
        return;
    }
    if (m_OnSelect) {
        m_OnSelect();
    }
}

bool CreateTemplateRow::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void CreateTemplateRow::Tick(float deltaTime) {
    (void)deltaTime;
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    m_HoverAnim = Animator::Damp(m_HoverAnim, IsHovered() && !m_Selected ? 1.0f : 0.0f, damp);
    Widget::Tick(deltaTime);
    InvalidateUI();
}

// ── WizardSeparator ──────────────────────────────────────────────────────────

Size WizardSeparator::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 100.0f * s,
        1.0f * s
    };
    return m_DesiredSize;
}

void WizardSeparator::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + allottedRect.height * 0.5f,
        allottedRect.width,
        m_DesiredSize.height
    };
}

void WizardSeparator::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, LColor(ColorToken::Separator));
}

} // namespace we::programs::welauncher
