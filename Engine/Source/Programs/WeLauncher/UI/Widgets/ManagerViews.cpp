#include "UI/Widgets/ManagerViews.h"

#include "UI/LauncherHelpers.h"

#include "KindUI/Core/Animator.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>
#include <cmath>

using namespace we::runtime::kindui;

namespace we::programs::welauncher {
namespace {

float ApproxTextWidth(const std::string& text, float textSize) {
    return static_cast<float>(text.size()) * textSize * 0.55f;
}

void PaintIconButton(
    PaintContext& context,
    const Rect& rect,
    const char* icon,
    bool hovered,
    bool accent,
    float radius) {
    const float s = LScale();
    if (hovered || accent) {
        Color bg = accent ? LColor(ColorToken::SelectedBackground) : LColor(ColorToken::HoverBackground);
        context.DrawRoundedRect(rect, bg, radius);
    }
    const float iconSize = 14.0f * s;
    IconPainter::DrawIcon(
        context,
        icon,
        Rect{
            rect.x + (rect.width - iconSize) * 0.5f,
            rect.y + (rect.height - iconSize) * 0.5f,
            iconSize,
            iconSize
        },
        accent ? LColor(ColorToken::AccentPrimary)
               : (hovered ? LColor(ColorToken::TextPrimary) : LColor(ColorToken::IconSecondary)));
}

void PaintStatusDot(PaintContext& context, const Rect& row, const std::string& status, float s) {
    Color c = LColor(ColorToken::TextMuted);
    if (status == "Compatible") {
        c = LColor(ColorToken::Success);
    } else if (status == "Warning") {
        c = LColor(ColorToken::Warning);
    } else if (status == "Incompatible") {
        c = LColor(ColorToken::ErrorForeground);
    }
    const float d = 6.0f * s;
    context.DrawRoundedRect(
        Rect{ row.x, row.y + (row.height - d) * 0.5f, d, d },
        c,
        d * 0.5f);
}

std::string Ellipsize(const std::string& text, float maxWidth, float textSize) {
    if (ApproxTextWidth(text, textSize) <= maxWidth) {
        return text;
    }
    std::string out = text;
    while (out.size() > 3 && ApproxTextWidth(out + "...", textSize) > maxWidth) {
        out.pop_back();
    }
    return out + "...";
}

} // namespace

const char* TemplateTypeIcon(const std::string& templateId) {
    if (templateId == "FirstPerson" || templateId == "ThirdPerson" || templateId == "TopDown"
        || templateId == "Vehicle") {
        return Icons::PlayName;
    }
    if (templateId == "VR" || templateId == "AR") {
        return Icons::Cube3DName;
    }
    if (templateId == "Blank" || templateId == "Empty") {
        return Icons::Cube3DName;
    }
    return Icons::LayersName;
}

Color TemplateBadgeColor(const std::string& templateId) {
    unsigned int hash = 2166136261u;
    for (unsigned char ch : templateId) {
        hash ^= ch;
        hash *= 16777619u;
    }
    const float h = static_cast<float>(hash % 360) / 360.0f;
    // Simple HSL-ish accent from hash (keep readable on dark UI).
    const float r = 0.35f + 0.45f * std::fabs(std::sin(h * 6.28318f));
    const float g = 0.35f + 0.45f * std::fabs(std::sin((h + 0.33f) * 6.28318f));
    const float b = 0.35f + 0.45f * std::fabs(std::sin((h + 0.66f) * 6.28318f));
    return Color{ r, g, b, 1.0f };
}

ProjectColumnLayout ProjectColumnLayout::Compute(float totalWidth, float scale) {
    ProjectColumnLayout layout;
    layout.favorite *= scale;
    layout.lastOpened *= scale;
    layout.engine *= scale;
    layout.platform *= scale;
    layout.status *= scale;
    layout.more *= scale;

    // Columns fill the table band; page host may add outer gutters for full-bleed headers.
    const float fixed = layout.favorite + layout.lastOpened + layout.engine + layout.platform
        + layout.status + layout.more;
    layout.name = std::max(180.0f * scale, totalWidth - fixed);
    return layout;
}

float TableContentInset(float scale) {
    return kLauncherContentPadX * scale;
}

ProjectColumnLayout TableColumnsForGeometry(float geometryWidth, float scale) {
    const float inset = TableContentInset(scale);
    return ProjectColumnLayout::Compute(std::max(0.0f, geometryWidth - inset * 2.0f), scale);
}

// ── ProjectTableHeader ───────────────────────────────────────────────────────

Size ProjectTableHeader::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 640.0f * s,
        kHeight * s
    };
    return m_DesiredSize;
}

void ProjectTableHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ProjectTableHeader::Paint(PaintContext& context) {
    const float s = LScale();
    const auto header = ThemeManager::Get().Resolve(StyleRole::TableHeader);
    context.DrawRect(m_Geometry, header.background);
    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f * s, m_Geometry.width, 1.0f * s },
        LColor(ColorToken::Separator));

    const float inset = TableContentInset(s);
    const auto cols = TableColumnsForGeometry(m_Geometry.width, s);
    const float textSize = header.fontSize > 0.0f ? header.fontSize : LMetric(MetricToken::TextSizeCaption) * s;
    float x = m_Geometry.x + inset;

    Color divider = LColor(ColorToken::Separator);
    divider.a *= 0.85f;
    auto drawDivider = [&](float atX) {
        context.DrawRect(
            Rect{ std::round(atX), m_Geometry.y + 6.0f * s, 1.0f * s, m_Geometry.height - 12.0f * s },
            divider);
    };

    auto drawCol = [&](const char* label, float width, bool sorted, bool withDivider) {
        if (width <= 1.0f) {
            return;
        }
        if (withDivider) {
            drawDivider(x);
        }
        const float textPad = withDivider ? 10.0f * s : 0.0f;
        std::string text = label;
        if (sorted && label[0] != '\0') {
            text += "  ^";
        }
        context.DrawText(
            text,
            Point{ x + textPad, m_Geometry.y + (m_Geometry.height - textSize) * 0.5f },
            sorted ? LColor(ColorToken::TextPrimary) : header.foreground,
            textSize,
            sorted);
        x += width;
    };

    // Atlas favorite glyph (Icons_Star → "star"), not the sun/light icon.
    const float starPx = 16.0f * s;
    IconPainter::DrawIcon(
        context,
        "star",
        Rect{
            x + (cols.favorite - starPx) * 0.5f,
            m_Geometry.y + (m_Geometry.height - starPx) * 0.5f,
            starPx,
            starPx
        },
        LColor(ColorToken::IconSecondary));
    x += cols.favorite;
    drawCol("NAME", cols.name, m_SortMode == ProjectSortMode::Name, true);
    drawCol("MODIFIED", cols.lastOpened, m_SortMode == ProjectSortMode::Recent, true);
    drawCol("EDITOR VERSION", cols.engine, m_SortMode == ProjectSortMode::Engine, true);
    drawCol("PLATFORM", cols.platform, false, true);
    drawCol("STATUS", cols.status, false, true);
}

void ProjectTableHeader::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_PressedCol = 1;
    }
}

void ProjectTableHeader::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left || m_PressedCol < 0) {
        m_PressedCol = -1;
        return;
    }
    m_PressedCol = -1;
    if (!m_Geometry.Contains(event.position) || !m_OnSort) {
        return;
    }
    const float s = LScale();
    const float inset = TableContentInset(s);
    const auto cols = TableColumnsForGeometry(m_Geometry.width, s);
    float x = m_Geometry.x + inset;
    const float local = event.position.x - x;
    float cursor = cols.favorite;
    if (local < cursor + cols.name) {
        m_OnSort(ProjectSortMode::Name);
        return;
    }
    cursor += cols.name;
    if (local < cursor + cols.lastOpened) {
        m_OnSort(ProjectSortMode::Recent);
        return;
    }
    cursor += cols.lastOpened;
    if (local < cursor + cols.engine) {
        m_OnSort(ProjectSortMode::Engine);
    }
}

bool ProjectTableHeader::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

// ── ProjectTableRow ──────────────────────────────────────────────────────────

ProjectTableRow::ProjectTableRow(ProjectSummary summary, bool selected, bool favorite)
    : m_Summary(std::move(summary))
    , m_Selected(selected)
    , m_Favorite(favorite) {
}

Size ProjectTableRow::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 640.0f * s,
        kRowH * s
    };
    return m_DesiredSize;
}

void ProjectTableRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

Rect ProjectTableRow::FavoriteRect() const {
    const float s = LScale();
    const float btn = 22.0f * s;
    const float inset = TableContentInset(s);
    const auto cols = TableColumnsForGeometry(m_Geometry.width, s);
    return Rect{
        m_Geometry.x + inset + (cols.favorite - btn) * 0.5f,
        m_Geometry.y + (m_Geometry.height - btn) * 0.5f,
        btn,
        btn
    };
}

Rect ProjectTableRow::MoreRect() const {
    const float s = LScale();
    const float btn = 22.0f * s;
    const float inset = TableContentInset(s);
    const auto cols = TableColumnsForGeometry(m_Geometry.width, s);
    return Rect{
        m_Geometry.x + inset + cols.favorite + cols.name + cols.lastOpened + cols.engine
            + cols.platform + cols.status + (cols.more - btn) * 0.5f,
        m_Geometry.y + (m_Geometry.height - btn) * 0.5f,
        btn,
        btn
    };
}

ProjectTableRow::HitZone ProjectTableRow::HitTest(const Point& p) const {
    if (FavoriteRect().Contains(p)) {
        return HitZone::Favorite;
    }
    if (MoreRect().Contains(p)) {
        return HitZone::More;
    }
    if (m_Geometry.Contains(p)) {
        return HitZone::Body;
    }
    return HitZone::None;
}

void ProjectTableRow::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = 0.0f;

    ControlChrome::InteractionState rowState{ m_HoverAnim, 0.0f, m_Selected, m_Focused, false };
    ControlChrome::PaintListRow(context, m_Geometry, rowState);

    const auto cols = TableColumnsForGeometry(m_Geometry.width, s);
    const float inset = TableContentInset(s);
    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    const float metaSize = LMetric(MetricToken::TextSizeCaption) * s;
    const float metaY = m_Geometry.y + (m_Geometry.height - metaSize) * 0.5f;
    const float nameGap = 2.0f * s;
    const float nameBlockH = textSize + nameGap + metaSize;
    const float nameTop = m_Geometry.y + (m_Geometry.height - nameBlockH) * 0.5f;

    PaintIconButton(
        context,
        FavoriteRect(),
        "star",
        m_HoverZone == HitZone::Favorite,
        m_Favorite,
        radius);

    float x = m_Geometry.x + inset + cols.favorite;
    const float colPad = 10.0f * s;

    const std::string displayName = m_Summary.descriptor.displayName.empty()
        ? m_Summary.descriptor.projectName
        : m_Summary.descriptor.displayName;
    context.DrawText(
        Ellipsize(displayName, cols.name - colPad - 8.0f * s, textSize),
        Point{ x + colPad, nameTop },
        LColor(ColorToken::TextPrimary),
        textSize,
        true);
    context.DrawText(
        Ellipsize(EllipsizePath(m_Summary.projectRoot, 72), cols.name - colPad - 8.0f * s, metaSize),
        Point{ x + colPad, nameTop + textSize + nameGap },
        LColor(ColorToken::TextMuted),
        metaSize);
    x += cols.name;

    context.DrawText(
        FormatRelativeTime(m_Summary.descriptor.lastOpenedUtc),
        Point{ x + colPad, metaY },
        LColor(ColorToken::TextSecondary),
        metaSize);
    x += cols.lastOpened;

    context.DrawText(
        Ellipsize(
            m_Summary.descriptor.engineVersion.empty() ? "—" : m_Summary.descriptor.engineVersion,
            cols.engine - colPad - 4.0f * s,
            metaSize),
        Point{ x + colPad, metaY },
        LColor(ColorToken::TextSecondary),
        metaSize);
    x += cols.engine;

    context.DrawText(
        Ellipsize(m_Summary.platforms.empty() ? "—" : m_Summary.platforms, cols.platform - colPad - 4.0f * s, metaSize),
        Point{ x + colPad, metaY },
        LColor(ColorToken::TextSecondary),
        metaSize);
    x += cols.platform;

    {
        const std::string& status = m_Summary.statusLabel.empty() ? "Unknown" : m_Summary.statusLabel;
        Color statusColor = LColor(ColorToken::TextMuted);
        if (status == "Compatible") {
            statusColor = LColor(ColorToken::Success);
        } else if (status == "Warning") {
            statusColor = LColor(ColorToken::Warning);
        } else if (status == "Incompatible") {
            statusColor = LColor(ColorToken::ErrorForeground);
        }
        const float d = 6.0f * s;
        context.DrawRoundedRect(
            Rect{ x + colPad, m_Geometry.y + (m_Geometry.height - d) * 0.5f, d, d },
            statusColor,
            d * 0.5f);
        context.DrawText(
            Ellipsize(status, cols.status - colPad - 14.0f * s, metaSize),
            Point{ x + colPad + 10.0f * s, metaY },
            LColor(ColorToken::TextSecondary),
            metaSize);
    }

    PaintIconButton(
        context,
        MoreRect(),
        Icons::MoreName,
        m_HoverZone == HitZone::More,
        false,
        radius);
}

void ProjectTableRow::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = HitTest(event.position);
        m_CtrlDown = event.ctrlDown;
    } else if (event.button == MouseButton::Right && m_Geometry.Contains(event.position)) {
        // Context menu via right-click
        m_Pressed = HitZone::More;
        m_CtrlDown = false;
    }
}

void ProjectTableRow::OnMouseMove(const MouseEvent& event) {
    const HitZone zone = HitTest(event.position);
    if (zone != m_HoverZone) {
        m_HoverZone = zone;
        InvalidateUI();
    }
    m_Hovered = zone != HitZone::None;
}

void ProjectTableRow::OnMouseUp(const MouseEvent& event) {
    const bool left = event.button == MouseButton::Left;
    const bool right = event.button == MouseButton::Right;
    if ((!left && !right) || m_Pressed == HitZone::None) {
        m_Pressed = HitZone::None;
        return;
    }

    if (right || m_Pressed == HitZone::More) {
        if (m_OnAction && (right ? m_Geometry.Contains(event.position) : HitTest(event.position) == HitZone::More)) {
            if (m_OnSelect) {
                m_OnSelect(false);
            }
            m_OnAction(ProjectCardAction::More);
        }
        m_Pressed = HitZone::None;
        return;
    }

    const HitZone hit = HitTest(event.position);
    if (hit != m_Pressed) {
        m_Pressed = HitZone::None;
        return;
    }
    m_Pressed = HitZone::None;

    if (hit == HitZone::Favorite) {
        if (m_OnAction) {
            m_OnAction(ProjectCardAction::Favorite);
        }
        return;
    }
    if (hit == HitZone::Body) {
        const auto now = std::chrono::steady_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastClick).count();
        const bool isDouble = ms > 0 && ms < 400;
        m_LastClick = now;
        if (m_OnSelect) {
            m_OnSelect(m_CtrlDown);
        }
        if (isDouble && m_OnAction) {
            m_OnAction(ProjectCardAction::Open);
        }
    }
}

bool ProjectTableRow::ShowsPointerCursor(const Point& position) const {
    return HitTest(position) != HitZone::None;
}

void ProjectTableRow::Tick(float deltaTime) {
    (void)deltaTime;
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    const bool hovered = m_HoverZone != HitZone::None && !m_Selected;
    m_HoverAnim = Animator::Damp(m_HoverAnim, hovered ? 1.0f : 0.0f, damp);
    Widget::Tick(deltaTime);
    InvalidateUI();
}

// ── TemplateListRow ──────────────────────────────────────────────────────────

TemplateListRow::TemplateListRow(ProjectTemplateInfo info, bool selected)
    : m_Info(std::move(info))
    , m_Selected(selected) {
}

Size TemplateListRow::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 480.0f * s,
        kRowH * s
    };
    return m_DesiredSize;
}

void TemplateListRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TemplateListRow::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;

    ControlChrome::InteractionState rowState{ m_HoverAnim, 0.0f, m_Selected, false, false };
    ControlChrome::PaintListRow(context, m_Geometry, rowState);
    if (m_Selected) {
        context.DrawRect(
            Rect{ m_Geometry.x, m_Geometry.y + 6.0f * s, 2.0f * s, m_Geometry.height - 12.0f * s },
            LColor(ColorToken::AccentPrimary));
    }

    const float iconBox = 28.0f * s;
    const Color badge = TemplateBadgeColor(m_Info.id);
    const float pad = 10.0f * s;
    context.DrawRoundedRect(
        Rect{ m_Geometry.x + pad, m_Geometry.y + (m_Geometry.height - iconBox) * 0.5f, iconBox, iconBox },
        Color::Lerp(badge, LColor(ColorToken::PanelBackground), 0.5f),
        6.0f * s);
    IconPainter::DrawIcon(
        context,
        TemplateTypeIcon(m_Info.id),
        Rect{
            m_Geometry.x + pad + 6.0f * s,
            m_Geometry.y + (m_Geometry.height - 16.0f * s) * 0.5f,
            16.0f * s,
            16.0f * s
        },
        badge);

    const float textX = m_Geometry.x + pad + iconBox + 10.0f * s;
    const float titleSize = LMetric(MetricToken::TextSizeBody) * s;
    const float metaSize = LMetric(MetricToken::TextSizeCaption) * s;
    context.DrawText(
        m_Info.displayName,
        Point{ textX, m_Geometry.y + 8.0f * s },
        LColor(ColorToken::TextPrimary),
        titleSize,
        true);

    std::string platforms;
    for (std::size_t i = 0; i < m_Info.platforms.size(); ++i) {
        if (i > 0) {
            platforms += ", ";
        }
        platforms += m_Info.platforms[i];
    }
    const std::string meta = (m_Info.category.empty() ? "Template" : m_Info.category)
        + "  ·  "
        + (m_Info.description.empty() ? "No description" : Ellipsize(m_Info.description, 280.0f * s, metaSize))
        + (platforms.empty() ? "" : ("  ·  " + platforms));
    context.DrawText(
        meta,
        Point{ textX, m_Geometry.y + 8.0f * s + titleSize + 2.0f * s },
        LColor(ColorToken::TextMuted),
        metaSize);
}

void TemplateListRow::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = m_Geometry.Contains(event.position);
    }
}

void TemplateListRow::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed && m_Geometry.Contains(event.position) && m_OnSelect) {
        m_OnSelect();
    }
    m_Pressed = false;
}

bool TemplateListRow::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void TemplateListRow::Tick(float deltaTime) {
    (void)deltaTime;
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered && !m_Selected ? 1.0f : 0.0f, damp);
    Widget::Tick(deltaTime);
    InvalidateUI();
}

// ── EngineInstallRow ─────────────────────────────────────────────────────────

EngineInstallRow::EngineInstallRow(EngineInstallInfo info, bool selected)
    : m_Info(std::move(info))
    , m_Selected(selected) {
}

Size EngineInstallRow::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 640.0f * s,
        kRowH * s
    };
    return m_DesiredSize;
}

void EngineInstallRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

Rect EngineInstallRow::ActionRect(int index) const {
    const float s = LScale();
    const float btn = 24.0f * s;
    const float gap = 4.0f * s;
    const float total = 5.0f * btn + 4.0f * gap;
    const float start = m_Geometry.x + m_Geometry.width - 8.0f * s - total;
    return Rect{
        start + static_cast<float>(index) * (btn + gap),
        m_Geometry.y + (m_Geometry.height - btn) * 0.5f,
        btn,
        btn
    };
}

EngineInstallRow::HitZone EngineInstallRow::HitTest(const Point& p) const {
    static const HitZone kZones[] = {
        HitZone::Launch, HitZone::Verify, HitZone::Repair, HitZone::Folder, HitZone::Uninstall
    };
    for (int i = 0; i < 5; ++i) {
        if (ActionRect(i).Contains(p)) {
            return kZones[i];
        }
    }
    if (m_Geometry.Contains(p)) {
        return HitZone::Body;
    }
    return HitZone::None;
}

void EngineInstallRow::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(MetricToken::CornerRadiusSmall) * s;

    ControlChrome::InteractionState rowState{ m_HoverAnim, 0.0f, m_Selected, false, false };
    ControlChrome::PaintListRow(context, m_Geometry, rowState);

    const float textSize = LMetric(MetricToken::TextSizeBody) * s;
    const float metaSize = LMetric(MetricToken::TextSizeCaption) * s;
    float x = m_Geometry.x + 12.0f * s;

    context.DrawText(
        m_Info.engineVersion.empty() ? "Unknown" : m_Info.engineVersion,
        Point{ x, m_Geometry.y + 8.0f * s },
        LColor(ColorToken::TextPrimary),
        textSize,
        true);
    context.DrawText(
        m_Info.buildId + "  ·  " + EllipsizePath(m_Info.engineRoot, 42)
            + "  ·  SDK " + m_Info.sdkStatus
            + "  ·  " + std::to_string(m_Info.pluginCount) + " plugins"
            + "  ·  " + m_Info.updateStatus,
        Point{ x, m_Geometry.y + 8.0f * s + textSize + 2.0f * s },
        LColor(ColorToken::TextMuted),
        metaSize);

    static const char* kIcons[] = {
        Icons::PlayName, Icons::CheckName, Icons::BuildName, Icons::OpenFolderName, Icons::DeleteName
    };
    static const HitZone kZones[] = {
        HitZone::Launch, HitZone::Verify, HitZone::Repair, HitZone::Folder, HitZone::Uninstall
    };
    for (int i = 0; i < 5; ++i) {
        PaintIconButton(
            context,
            ActionRect(i),
            kIcons[i],
            m_HoverZone == kZones[i],
            false,
            radius);
    }
}

void EngineInstallRow::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = HitTest(event.position);
    }
}

void EngineInstallRow::OnMouseMove(const MouseEvent& event) {
    const HitZone zone = HitTest(event.position);
    if (zone != m_HoverZone) {
        m_HoverZone = zone;
        InvalidateUI();
    }
    m_Hovered = zone != HitZone::None;
}

void EngineInstallRow::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left || m_Pressed == HitZone::None) {
        m_Pressed = HitZone::None;
        return;
    }
    const HitZone hit = HitTest(event.position);
    if (hit != m_Pressed) {
        m_Pressed = HitZone::None;
        return;
    }
    m_Pressed = HitZone::None;
    if (hit == HitZone::Body) {
        if (m_OnSelect) {
            m_OnSelect();
        }
        return;
    }
    if (!m_OnAction) {
        return;
    }
    switch (hit) {
    case HitZone::Launch: m_OnAction("launch"); break;
    case HitZone::Verify: m_OnAction("verify"); break;
    case HitZone::Repair: m_OnAction("repair"); break;
    case HitZone::Folder: m_OnAction("folder"); break;
    case HitZone::Uninstall: m_OnAction("uninstall"); break;
    default: break;
    }
}

bool EngineInstallRow::ShowsPointerCursor(const Point& position) const {
    return HitTest(position) != HitZone::None;
}

void EngineInstallRow::Tick(float deltaTime) {
    (void)deltaTime;
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_HoverZone != HitZone::None && !m_Selected ? 1.0f : 0.0f, damp);
    Widget::Tick(deltaTime);
    InvalidateUI();
}

// ── SimpleColumnHeader ───────────────────────────────────────────────────────

SimpleColumnHeader::SimpleColumnHeader(std::vector<std::string> columns)
    : m_Columns(std::move(columns)) {
}

Size SimpleColumnHeader::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 640.0f * s,
        kHeight * s
    };
    return m_DesiredSize;
}

void SimpleColumnHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void SimpleColumnHeader::Paint(PaintContext& context) {
    const float s = LScale();
    const auto header = ThemeManager::Get().Resolve(StyleRole::TableHeader);
    context.DrawRect(m_Geometry, header.background);
    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f * s, m_Geometry.width, 1.0f * s },
        LColor(ColorToken::Separator));

    if (m_Columns.empty()) {
        return;
    }

    const float inset = TableContentInset(s);
    const float textSize = header.fontSize > 0.0f ? header.fontSize : LMetric(MetricToken::TextSizeCaption) * s;
    const float band = std::max(0.0f, m_Geometry.width - inset * 2.0f);
    const std::size_t count = m_Columns.size();
    const float colW = band / static_cast<float>(count);
    float x = m_Geometry.x + inset;

    Color divider = LColor(ColorToken::Separator);
    divider.a *= 0.85f;

    for (std::size_t i = 0; i < count; ++i) {
        if (i > 0) {
            context.DrawRect(
                Rect{ std::round(x), m_Geometry.y + 6.0f * s, 1.0f * s, m_Geometry.height - 12.0f * s },
                divider);
        }
        const float textPad = i > 0 ? 10.0f * s : 0.0f;
        context.DrawText(
            m_Columns[i],
            Point{ x + textPad, m_Geometry.y + (m_Geometry.height - textSize) * 0.5f },
            header.foreground,
            textSize);
        x += colW;
    }
}

// ── LibraryPackageRow ────────────────────────────────────────────────────────

LibraryPackageRow::LibraryPackageRow(
    std::string kind,
    std::string name,
    std::string detail,
    const char* icon,
    ClickFn onClick)
    : m_Kind(std::move(kind))
    , m_Name(std::move(name))
    , m_Detail(std::move(detail))
    , m_Icon(icon)
    , m_OnClick(std::move(onClick)) {
}

Size LibraryPackageRow::Measure(const Size& availableSize) {
    const float s = LScale();
    m_DesiredSize = Size{
        availableSize.width > 0.0f ? availableSize.width : 480.0f * s,
        kRowH * s
    };
    return m_DesiredSize;
}

void LibraryPackageRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void LibraryPackageRow::Paint(PaintContext& context) {
    const float s = LScale();
    ControlChrome::InteractionState rowState{ m_HoverAnim, 0.0f, false, false, false };
    ControlChrome::PaintListRow(context, m_Geometry, rowState);

    const float pad = 10.0f * s;
    const float iconSize = 16.0f * s;
    if (m_Icon) {
        IconPainter::DrawIcon(
            context,
            m_Icon,
            Rect{
                m_Geometry.x + pad,
                m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f,
                iconSize,
                iconSize
            },
            LColor(ColorToken::IconSecondary));
    }

    const float textX = m_Geometry.x + pad + iconSize + 10.0f * s;
    const float titleSize = LMetric(MetricToken::TextSizeBody) * s;
    const float metaSize = LMetric(MetricToken::TextSizeCaption) * s;
    context.DrawText(
        m_Name,
        Point{ textX, m_Geometry.y + (m_Geometry.height - titleSize) * 0.5f - (m_Detail.empty() ? 0.0f : 6.0f * s) },
        LColor(ColorToken::TextPrimary),
        titleSize,
        true);
    if (!m_Detail.empty()) {
        context.DrawText(
            m_Kind + "  ·  " + m_Detail,
            Point{ textX, m_Geometry.y + m_Geometry.height * 0.5f + 2.0f * s },
            LColor(ColorToken::TextMuted),
            metaSize);
    }
}

void LibraryPackageRow::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = m_Geometry.Contains(event.position);
    }
}

void LibraryPackageRow::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed && m_Geometry.Contains(event.position) && m_OnClick) {
        m_OnClick();
    }
    m_Pressed = false;
}

bool LibraryPackageRow::ShowsPointerCursor(const Point& position) const {
    return m_OnClick && m_Geometry.Contains(position);
}

void LibraryPackageRow::Tick(float deltaTime) {
    (void)deltaTime;
    const float damp = LMetric(MetricToken::HoverAnimationDamping);
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, damp);
    Widget::Tick(deltaTime);
    InvalidateUI();
}

} // namespace we::programs::welauncher
