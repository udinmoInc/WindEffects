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
    const float pad = LMetric(MetricToken::Space3) * s;
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
    const float pad = LMetric(MetricToken::Space3) * s;
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
    const float radius = LMetric(MetricToken::CornerRadiusMedium) * s;
    context.DrawRoundedRect(m_Geometry, LColor(ColorToken::CardBackground), radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ColorToken::BorderSubtle), 1.0f, radius);

    const float pad = LMetric(MetricToken::Space3) * s;
    const float titleSize = LMetric(MetricToken::TextSizeHeader) * s;
    context.DrawText(
        m_Title,
        Point{ m_Geometry.x + pad, m_Geometry.y + (m_HeaderHeight * s - titleSize) * 0.5f },
        LColor(ColorToken::TextPrimary),
        titleSize,
        true);

    if (m_Content && m_Content->IsVisible()) {
        m_Content->Paint(context);
    }
}

void SectionCard::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

// ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ EmptyStatePanel ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬ÃƒÂ¢Ã¢â‚¬ÂÃ¢â€šÂ¬

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
    const float btnH = LMetric(MetricToken::HeaderControlHeight) * s;
    const float gap = LMetric(MetricToken::Space2) * s;
    const bool hasPrimary = !m_PrimaryLabel.empty();
    const bool hasSecondary = !m_SecondaryLabel.empty();
    const bool hasIcon = m_Icon && m_Icon[0];
    const bool hasSubtitle = !m_Subtitle.empty();

    const float primaryW = hasPrimary
        ? std::max(140.0f * s, ApproxTextWidth(m_PrimaryLabel, LMetric(MetricToken::TextSizeToolbar) * s) + 48.0f * s)
        : 0.0f;
    const float secondaryW = hasSecondary
        ? std::max(160.0f * s, ApproxTextWidth(m_SecondaryLabel, LMetric(MetricToken::TextSizeToolbar) * s) + 48.0f * s)
        : 0.0f;

    float totalW = primaryW + secondaryW;
    if (hasPrimary && hasSecondary) {
        totalW += gap;
    }

    const float titleSize = LMetric(MetricToken::TextSizeHeader) * s;
    const float subSize = LMetric(MetricToken::TextSizeBody) * s;
    const float iconSize = hasIcon ? 72.0f * s : 0.0f;
    float blockH = titleSize;
    if (hasIcon) {
        blockH += iconSize + LMetric(MetricToken::Space3) * s;
    }
    if (hasSubtitle) {
        blockH += LMetric(MetricToken::Space2) * s + subSize;
    }
    if (hasPrimary || hasSecondary) {
        blockH += LMetric(MetricToken::Space4) * s + btnH;
    }

    const float blockTop = m_Geometry.y + std::max(24.0f * s, (m_Geometry.height - blockH) * 0.5f);
    float y = blockTop;
    if (hasIcon) {
        y += iconSize + LMetric(MetricToken::Space3) * s;
    }
    y += titleSize;
    if (hasSubtitle) {
        y += LMetric(MetricToken::Space2) * s + subSize;
    }
    y += LMetric(MetricToken::Space4) * s;
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
    context.DrawRect(m_Geometry, LColor(ColorToken::PanelContentBackground));

    const float cx = m_Geometry.x + m_Geometry.width * 0.5f;
    const bool hasIcon = m_Icon && m_Icon[0];
    const bool hasSubtitle = !m_Subtitle.empty();
    const bool hasActions = !m_PrimaryLabel.empty() || !m_SecondaryLabel.empty();

    const float titleSize = LMetric(MetricToken::TextSizeHeader) * s;
    const float subSize = LMetric(MetricToken::TextSizeBody) * s;
    const float iconSize = hasIcon ? 72.0f * s : 0.0f;
    float blockH = titleSize;
    if (hasIcon) {
        blockH += iconSize + LMetric(MetricToken::Space3) * s;
    }
    if (hasSubtitle) {
        blockH += LMetric(MetricToken::Space2) * s + subSize;
    }
    if (hasActions) {
        blockH += LMetric(MetricToken::Space4) * s + 36.0f * s;
    }
    const float blockTop = m_Geometry.y + std::max(24.0f * s, (m_Geometry.height - blockH) * 0.5f);

    float y = blockTop;
    if (hasIcon) {
        Color iconColor = LColor(ColorToken::IconPrimary);
        iconColor.a *= 0.45f;
        IconPainter::DrawIcon(
            context,
            m_Icon,
            Rect{ cx - iconSize * 0.5f, y, iconSize, iconSize },
            iconColor);
        y += iconSize + LMetric(MetricToken::Space3) * s;
    }

    context.DrawText(
        m_Title,
        Point{ cx - context.GetTextWidth(m_Title, titleSize, true) * 0.5f, y },
        LColor(ColorToken::TextPrimary),
        titleSize,
        true);
    y += titleSize;

    if (hasSubtitle) {
        y += LMetric(MetricToken::Space2) * s;
        context.DrawText(
            m_Subtitle,
            Point{ cx - context.GetTextWidth(m_Subtitle, subSize) * 0.5f, y },
            LColor(ColorToken::TextSecondary),
            subSize);
        y += subSize;
    }

    auto paintButton = [&](const Rect& r, const std::string& label, const char* icon, bool primary, bool hovered, bool pressed) {
        if (r.width <= 0.0f) {
            return;
        }
        const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;
        Color bg = primary
            ? LColor(ColorToken::ButtonPrimaryBackground)
            : LColor(ColorToken::ActiveBackground);
        if (primary) {
            if (pressed) {
                bg = LColor(ColorToken::ButtonPrimaryPressed);
            } else if (hovered) {
                bg = Color::Lerp(bg, LColor(ColorToken::ButtonPrimaryHover), 0.85f);
            }
        } else if (hovered) {
            bg = Color::Lerp(bg, LColor(ColorToken::HoverBackground), 0.75f);
        }
        context.DrawRoundedRect(r, bg, radius);
        if (!primary) {
            context.DrawRoundedRectOutline(r, LColor(ColorToken::BorderDefault), 1.0f, radius);
        }

        const float glyph = 14.0f * s;
        const float textSize = LMetric(MetricToken::TextSizeToolbar) * s;
        float contentW = context.GetTextWidth(label, textSize);
        if (icon && icon[0]) {
            contentW += glyph + 6.0f * s;
        }
        float x = r.x + (r.width - contentW) * 0.5f;
        const Color fg = primary
            ? Color{ 1.0f, 1.0f, 1.0f, 1.0f }
            : LColor(ColorToken::TextPrimary);
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

// --- ProjectsEmptyState -------------------------------------------------------

void ProjectsEmptyState::SetFolderPath(std::string path) {
    if (m_FolderPath == path) {
        return;
    }
    m_FolderPath = std::move(path);
    InvalidateUI();
}

Size ProjectsEmptyState::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 640.0f * s,
        availableSize.height > 0.0f ? availableSize.height : 420.0f * s
    };
    return m_DesiredSize;
}

void ProjectsEmptyState::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    LayoutContent();
}

void ProjectsEmptyState::LayoutContent() {
    const float s = LScale();
    const float btnH = 34.0f * s;
    const float newW = 148.0f * s;
    const float openW = 148.0f * s;
    const float gap = 8.0f * s;

    const float iconSize = 68.0f * s;
    const float blockH = iconSize + 16.0f * s + 28.0f * s + 8.0f * s + 40.0f * s
        + 16.0f * s + btnH + 28.0f * s + 52.0f * s;
    const float blockTop = m_Geometry.y + std::max(16.0f * s, (m_Geometry.height - blockH) * 0.5f);
    const float cx = m_Geometry.x + m_Geometry.width * 0.5f;

    const float btnY = blockTop + iconSize + 16.0f * s + 28.0f * s + 8.0f * s + 40.0f * s + 16.0f * s;
    const float totalBtnW = newW + gap + openW;
    float bx = cx - totalBtnW * 0.5f;
    m_NewRect = Rect{ bx, btnY, newW, btnH };
    bx += newW + gap;
    m_OpenRect = Rect{ bx, btnY, openW, btnH };
    // m_ChangeRect is finalized in Paint() once text widths are known.
}

ProjectsEmptyState::HitZone ProjectsEmptyState::HitTest(const Point& p) const {
    if (m_NewRect.Contains(p)) return HitZone::New;
    if (m_OpenRect.Contains(p)) return HitZone::Open;
    if (m_ChangeRect.Contains(p)) return HitZone::Change;
    return HitZone::None;
}

void ProjectsEmptyState::Paint(PaintContext& context) {
    const float s = LScale();
    context.DrawRect(m_Geometry, LColor(ColorToken::PanelContentBackground));

    LayoutContent();
    const float cx = m_Geometry.x + m_Geometry.width * 0.5f;
    const float iconSize = 68.0f * s;
    const float blockH = iconSize + 16.0f * s + 28.0f * s + 8.0f * s + 40.0f * s
        + 16.0f * s + 34.0f * s + 28.0f * s + 52.0f * s;
    const float blockTop = m_Geometry.y + std::max(16.0f * s, (m_Geometry.height - blockH) * 0.5f);

    IconPainter::DrawIcon(
        context,
        Icons::OpenFolderName,
        Rect{ cx - iconSize * 0.5f, blockTop, iconSize, iconSize },
        LColor(ColorToken::IconPrimary));

    float y = blockTop + iconSize + 16.0f * s;
    const float titleSize = LMetric(MetricToken::TextSizeHeader) * s;
    const std::string title = "No projects yet";
    context.DrawText(
        title,
        Point{ cx - context.GetTextWidth(title, titleSize, true) * 0.5f, y },
        LColor(ColorToken::TextPrimary),
        titleSize,
        true);

    y += titleSize + 8.0f * s;
    const float subSize = LMetric(MetricToken::TextSizeBody) * s;
    const std::string subtitle = "Create your first WindEffects project or open an existing project.";
    context.DrawText(
        subtitle,
        Point{ cx - context.GetTextWidth(subtitle, subSize) * 0.5f, y },
        LColor(ColorToken::TextSecondary),
        subSize);

    auto paintBtn = [&](const Rect& r, const char* label, bool primary, bool hovered, bool pressed) {
        const float radius = 8.0f * s;
        Color bg = primary
            ? LColor(ColorToken::ButtonPrimaryBackground)
            : LColor(ColorToken::ActiveBackground);
        if (primary) {
            if (pressed) bg = LColor(ColorToken::ButtonPrimaryPressed);
            else if (hovered) bg = LColor(ColorToken::ButtonPrimaryHover);
        } else if (hovered) {
            bg = Color::Lerp(bg, LColor(ColorToken::HoverBackground), 0.7f);
        }
        context.DrawRoundedRect(r, bg, radius);
        if (!primary) {
            context.DrawRoundedRectOutline(r, LColor(ColorToken::BorderDefault), 1.0f, radius);
        }
        const float textSize = LMetric(MetricToken::TextSizeToolbar) * s;
        const Color fg = primary ? Color{ 1.0f, 1.0f, 1.0f, 1.0f } : LColor(ColorToken::TextPrimary);
        const float tw = context.GetTextWidth(label, textSize);
        context.DrawText(
            label,
            Point{ r.x + (r.width - tw) * 0.5f, r.y + (r.height - textSize) * 0.5f },
            fg,
            textSize);
    };

    paintBtn(m_NewRect, "New Project", true, m_Hover == HitZone::New, m_Pressed == HitZone::New);
    paintBtn(m_OpenRect, "Open Project", false, m_Hover == HitZone::Open, m_Pressed == HitZone::Open);

    y = m_NewRect.y + m_NewRect.height + 28.0f * s;
    const float capSize = LMetric(MetricToken::TextSizeCaption) * s;
    const std::string cap = "Default Projects Folder";
    context.DrawText(
        cap,
        Point{ cx - context.GetTextWidth(cap, capSize) * 0.5f, y },
        LColor(ColorToken::TextCaption),
        capSize);

    y += capSize + 6.0f * s;
    const float pathSize = LMetric(MetricToken::TextSizeBody) * s;
    const std::string path = m_FolderPath.empty() ? "-" : EllipsizePath(m_FolderPath, 64);
    const float pathW = context.GetTextWidth(path, pathSize);
    const float changeGap = 12.0f * s;
    const float changeW = 72.0f * s;
    const float changeH = 28.0f * s;
    const float rowW = pathW + changeGap + changeW;
    const float rowX = cx - rowW * 0.5f;
    const float pathY = y + (changeH - pathSize) * 0.5f;
    context.DrawText(path, Point{ rowX, pathY }, LColor(ColorToken::TextSecondary), pathSize);

    m_ChangeRect = Rect{ rowX + pathW + changeGap, y, changeW, changeH };

    const float radius = 6.0f * s;
    Color changeBg = LColor(ColorToken::ActiveBackground);
    if (m_Hover == HitZone::Change) {
        changeBg = Color::Lerp(changeBg, LColor(ColorToken::HoverBackground), 0.75f);
    }
    context.DrawRoundedRect(m_ChangeRect, changeBg, radius);
    context.DrawRoundedRectOutline(m_ChangeRect, LColor(ColorToken::BorderDefault), 1.0f, radius);
    const float changeTextSize = LMetric(MetricToken::TextSizeCaption) * s;
    const char* changeLabel = "Change";
    context.DrawText(
        changeLabel,
        Point{
            m_ChangeRect.x + (m_ChangeRect.width - context.GetTextWidth(changeLabel, changeTextSize)) * 0.5f,
            m_ChangeRect.y + (m_ChangeRect.height - changeTextSize) * 0.5f
        },
        LColor(ColorToken::TextPrimary),
        changeTextSize);
}

void ProjectsEmptyState::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = HitTest(event.position);
    }
}

void ProjectsEmptyState::OnMouseMove(const MouseEvent& event) {
    const HitZone next = HitTest(event.position);
    if (next != m_Hover) {
        m_Hover = next;
        InvalidateUI();
    }
    m_Hovered = m_Hover != HitZone::None;
}

void ProjectsEmptyState::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const HitZone hit = HitTest(event.position);
    if (hit == m_Pressed) {
        if (hit == HitZone::New && m_OnNew) m_OnNew();
        else if (hit == HitZone::Open && m_OnOpen) m_OnOpen();
        else if (hit == HitZone::Change && m_OnChangeFolder) m_OnChangeFolder();
    }
    m_Pressed = HitZone::None;
}

bool ProjectsEmptyState::ShowsPointerCursor(const Point& position) const {
    return HitTest(position) != HitZone::None;
}


// Ã¢â€â‚¬Ã¢â€â‚¬ CompactSearchField Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬Ã¢â€â‚¬

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
    // Fixed size â€” never stretch with the parent.
    m_DesiredSize = Size{ kLauncherSearchW * s, kLauncherSearchH * s };
    return m_DesiredSize;
}

void CompactSearchField::Arrange(const Rect& allottedRect) {
    const float s = LScale();
    const float w = kLauncherSearchW * s;
    const float h = kLauncherSearchH * s;
    // Keep fixed width; right-align inside the allotted slot if the parent over-allocates.
    const float x = allottedRect.width > w
        ? allottedRect.x + allottedRect.width - w
        : allottedRect.x;
    const float y = allottedRect.y + (allottedRect.height - h) * 0.5f;
    m_Geometry = Rect{ x, y, w, h };
}

void CompactSearchField::Paint(PaintContext& context) {
    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, false, m_Focused, false };
    ControlChrome::PaintInputFrame(context, m_Geometry, state);

    const float s = LScale();
    const float iconSize = kLauncherIconPx * s;
    const float pad = 10.0f * s;
    IconPainter::DrawIcon(
        context,
        Icons::SearchName,
        Rect{
            std::round(m_Geometry.x + pad),
            std::round(m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f),
            iconSize,
            iconSize
        },
        LColor(ColorToken::IconSecondary));

    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    const bool empty = m_Text.empty();
    const std::string& draw = empty ? m_Placeholder : m_Text;
    context.DrawText(
        draw,
        Point{
            std::round(m_Geometry.x + pad + iconSize + kLauncherNavIconTextGap * s),
            std::round(m_Geometry.y + (m_Geometry.height - textSize) * 0.5f)
        },
        empty ? LColor(ColorToken::SearchPlaceholder) : LColor(ColorToken::TextPrimary),
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
        LMetric(MetricToken::HoverAnimationDamping));
    Widget::Tick(deltaTime);
}

bool CompactSearchField::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

} // namespace we::programs::welauncher
