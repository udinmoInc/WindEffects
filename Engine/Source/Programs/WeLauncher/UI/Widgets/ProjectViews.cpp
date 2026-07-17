#include "UI/Widgets/ProjectViews.h"

#include "UI/LauncherHelpers.h"

#include "KindUI/Core/Animator.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeToken.h"

#include <algorithm>
#include <cmath>
#include <memory>

using namespace we::runtime::kindui;

namespace we::programs::welauncher {
namespace {

float ApproxTextWidth(const std::string& text, float textSize) {
    return static_cast<float>(text.size()) * textSize * 0.55f;
}

int EstimateWrappedLines(const std::string& text, float fontSize, float maxWidth) {
    if (text.empty() || maxWidth <= 1.0f) {
        return 1;
    }
    const float charW = std::max(1.0f, fontSize * 0.55f);
    const int charsPerLine = std::max(1, static_cast<int>(maxWidth / charW));
    return std::max(1, static_cast<int>((text.size() + static_cast<std::size_t>(charsPerLine) - 1)
        / static_cast<std::size_t>(charsPerLine)));
}

void PaintPrimaryPill(
    PaintContext& context,
    const Rect& rect,
    const std::string& label,
    bool hovered,
    bool pressed) {
    const float s = LScale();
    const float radius = rect.height * 0.5f;
    Color bg = LColor(ThemeToken::ButtonPrimaryBackground);
    if (pressed) {
        bg = LColor(ThemeToken::ButtonPrimaryPressed);
    } else if (hovered) {
        bg = Color::Lerp(bg, LColor(ThemeToken::ButtonPrimaryHover), 0.65f);
    }
    context.DrawRoundedRect(rect, bg, radius);
    Color outline = LColor(ThemeToken::AccentPrimary);
    outline.a = 0.45f;
    context.DrawRoundedRectOutline(rect, outline, 1.0f, radius);

    const float textSize = LMetric(ThemeToken::TextSizeToolbar) * s;
    const float textW = ApproxTextWidth(label, textSize);
    context.DrawText(
        label,
        Point{
            rect.x + (rect.width - textW) * 0.5f,
            rect.y + (rect.height - textSize) * 0.5f
        },
        LColor(ThemeToken::TextPrimary),
        textSize,
        true);
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
        Color bg = accent ? LColor(ThemeToken::SelectedBackground) : LColor(ThemeToken::HoverBackground);
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
        accent ? LColor(ThemeToken::AccentPrimary)
               : (hovered ? LColor(ThemeToken::TextPrimary) : LColor(ThemeToken::IconSecondary)));
}

} // namespace

// â”€â”€ ProjectCard â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

ProjectCard::ProjectCard(ProjectSummary summary, bool selected, bool favorite)
    : m_Summary(std::move(summary))
    , m_Selected(selected)
    , m_Favorite(favorite) {
    BeginThumbnailLoad(0.08f);
}

void ProjectCard::SetThumbnail(
    we::rhi::RHIDescriptorSetHandle texture,
    ThumbnailVisualState state) {
    m_ThumbTexture = texture;
    m_ThumbState = state;
    m_ThumbDelay = 0.0f;
    if (state == ThumbnailVisualState::Ready && texture != we::rhi::RHIDescriptorSetHandle::Invalid) {
        m_ThumbFade = 0.0f;
    } else if (state == ThumbnailVisualState::Placeholder) {
        m_ThumbFade = 0.0f;
    } else {
        m_ThumbFade = 1.0f;
    }
    InvalidateUI();
}

void ProjectCard::BeginThumbnailLoad(float staggerSeconds) {
    m_ThumbState = ThumbnailVisualState::Skeleton;
    m_ThumbTexture = we::rhi::RHIDescriptorSetHandle::Invalid;
    m_ThumbFade = 1.0f;
    m_ThumbDelay = staggerSeconds;
    m_ThumbAge = 0.0f;
}

void ProjectCard::PaintThumb(PaintContext& context, const Rect& thumb, float radius) {
    switch (m_ThumbState) {
    case ThumbnailVisualState::Skeleton:
        PaintSkeletonBone(context, thumb, radius, 0.15f);
        break;
    case ThumbnailVisualState::Ready:
        if (m_ThumbTexture != we::rhi::RHIDescriptorSetHandle::Invalid) {
            PaintProjectThumbnailPlaceholder(
                context,
                thumb,
                radius,
                m_Summary.descriptor.displayName,
                m_Summary.descriptor.templateId,
                m_Summary.compatible,
                1.0f);
            Color tint = Color::White();
            tint.a = std::clamp(m_ThumbFade, 0.0f, 1.0f);
            context.DrawTexture(thumb, m_ThumbTexture, tint);
        } else {
            PaintProjectThumbnailPlaceholder(
                context,
                thumb,
                radius,
                m_Summary.descriptor.displayName,
                m_Summary.descriptor.templateId,
                m_Summary.compatible,
                std::clamp(m_ThumbFade, 0.0f, 1.0f));
        }
        break;
    case ThumbnailVisualState::Placeholder:
    default:
        PaintProjectThumbnailPlaceholder(
            context,
            thumb,
            radius,
            m_Summary.descriptor.displayName,
            m_Summary.descriptor.templateId,
            m_Summary.compatible,
            std::clamp(m_ThumbFade, 0.0f, 1.0f));
        break;
    }
}

Size ProjectCard::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    m_DesiredSize = Size{ kCardWidth * s, kCardHeight * s };
    return m_DesiredSize;
}

void ProjectCard::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

Rect ProjectCard::FavoriteRect() const {
    const float s = LScale();
    const float pad = 12.0f * s;
    const float btn = 26.0f * s;
    const float inset = 4.0f * s;
    return Rect{
        m_Geometry.x + m_Geometry.width - pad - btn - inset,
        m_Geometry.y + pad + inset,
        btn,
        btn
    };
}

Rect ProjectCard::OpenRect() const {
    const float s = LScale();
    const float pad = 12.0f * s;
    const float h = 28.0f * s;
    const float w = 68.0f * s;
    return Rect{
        m_Geometry.x + pad,
        m_Geometry.y + m_Geometry.height - pad - h,
        w,
        h
    };
}

Rect ProjectCard::MoreRect() const {
    const float s = LScale();
    const float pad = 12.0f * s;
    const float btn = 28.0f * s;
    return Rect{
        m_Geometry.x + m_Geometry.width - pad - btn,
        m_Geometry.y + m_Geometry.height - pad - btn,
        btn,
        btn
    };
}

ProjectCard::HitZone ProjectCard::HitTest(const Point& p) const {
    if (FavoriteRect().Contains(p)) {
        return HitZone::Favorite;
    }
    if (OpenRect().Contains(p)) {
        return HitZone::Open;
    }
    if (MoreRect().Contains(p)) {
        return HitZone::More;
    }
    if (m_Geometry.Contains(p)) {
        return HitZone::Body;
    }
    return HitZone::None;
}

void ProjectCard::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(ThemeToken::CornerRadiusMedium) * s;

    Color bg = LColor(ThemeToken::PanelBackground);
    if (m_Selected) {
        bg = LColor(ThemeToken::SelectedBackground);
    } else if (m_HoverAnim > 0.01f) {
        bg = Color::Lerp(bg, LColor(ThemeToken::HoverBackground), m_HoverAnim * 0.55f);
    }

    const float shadowLift = (2.0f + 4.0f * m_HoverAnim) * s;
    const float shadowBlur = (8.0f + 6.0f * m_HoverAnim) * s;
    Color shadow = LColor(ThemeToken::ShadowSubtle);
    shadow.a *= 0.55f + 0.45f * m_HoverAnim;
    context.DrawShadow(
        Rect{ m_Geometry.x, m_Geometry.y + shadowLift, m_Geometry.width, m_Geometry.height },
        shadow,
        radius,
        shadowBlur);
    context.DrawRoundedRect(m_Geometry, bg, radius);

    Color border = m_Selected ? LColor(ThemeToken::AccentPrimary) : LColor(ThemeToken::BorderDefault);
    context.DrawRoundedRectOutline(m_Geometry, border, m_Selected ? 1.5f : 1.0f, radius);

    const float pad = 12.0f * s;
    Rect thumb{
        m_Geometry.x + pad,
        m_Geometry.y + pad,
        m_Geometry.width - pad * 2.0f,
        112.0f * s
    };
    PaintThumb(context, thumb, radius - 2.0f);

    PaintIconButton(
        context,
        FavoriteRect(),
        Icons::StarName,
        m_HoverZone == HitZone::Favorite,
        m_Favorite,
        LMetric(ThemeToken::CornerRadiusSmall) * s);

    const float titleSize = LMetric(ThemeToken::TextSizeBody) * s;
    const float metaSize = LMetric(ThemeToken::TextSizeCaption) * s;
    float y = thumb.y + thumb.height + 10.0f * s;

    context.DrawText(
        m_Summary.descriptor.displayName,
        Point{ m_Geometry.x + pad, y },
        LColor(ThemeToken::TextPrimary),
        titleSize,
        true);
    y += titleSize + 4.0f * s;

    const std::string meta = m_Summary.descriptor.engineVersion
        + "  Â·  " + FormatRelativeTime(m_Summary.descriptor.lastOpenedUtc);
    context.DrawText(meta, Point{ m_Geometry.x + pad, y }, LColor(ThemeToken::TextMuted), metaSize);
    y += metaSize + 3.0f * s;

    context.DrawText(
        EllipsizePath(m_Summary.projectRoot, 34),
        Point{ m_Geometry.x + pad, y },
        LColor(ThemeToken::TextDisabled),
        metaSize);

    PaintPrimaryPill(
        context,
        OpenRect(),
        "Open",
        m_HoverZone == HitZone::Open,
        m_Pressed == HitZone::Open);

    PaintIconButton(
        context,
        MoreRect(),
        Icons::MoreName,
        m_HoverZone == HitZone::More,
        false,
        LMetric(ThemeToken::CornerRadiusSmall) * s);
}

void ProjectCard::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = HitTest(event.position);
    }
}

void ProjectCard::OnMouseMove(const MouseEvent& event) {
    const HitZone zone = HitTest(event.position);
    if (zone != m_HoverZone) {
        m_HoverZone = zone;
        InvalidateUI();
    }
    m_Hovered = zone != HitZone::None;
}

void ProjectCard::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left || m_Pressed == HitZone::None) {
        m_Pressed = HitZone::None;
        return;
    }
    const HitZone hit = HitTest(event.position);
    if (hit == m_Pressed) {
        switch (hit) {
        case HitZone::Body:
            if (m_OnSelect) {
                m_OnSelect();
            }
            break;
        case HitZone::Favorite:
            if (m_OnAction) {
                m_OnAction(ProjectCardAction::Favorite);
            }
            break;
        case HitZone::Open:
            if (m_OnSelect) {
                m_OnSelect();
            }
            if (m_OnAction) {
                m_OnAction(ProjectCardAction::Open);
            }
            break;
        case HitZone::More:
            if (m_OnSelect) {
                m_OnSelect();
            }
            if (m_OnAction) {
                m_OnAction(ProjectCardAction::More);
            }
            break;
        default:
            break;
        }
    }
    m_Pressed = HitZone::None;
}

bool ProjectCard::ShowsPointerCursor(const Point& position) const {
    return HitTest(position) != HitZone::None;
}

void ProjectCard::Tick(float deltaTime) {
    AdvanceSkeletonShimmer(deltaTime);

    m_ThumbAge += deltaTime;
    bool thumbDirty = false;
    if (m_ThumbState == ThumbnailVisualState::Skeleton) {
        if (m_ThumbAge >= m_ThumbDelay) {
            // No cached thumbnail yet â€” settle on intentional placeholder art.
            m_ThumbState = ThumbnailVisualState::Placeholder;
            m_ThumbFade = 0.0f;
            thumbDirty = true;
        } else {
            thumbDirty = true;
        }
    } else if (m_ThumbState == ThumbnailVisualState::Placeholder
        || m_ThumbState == ThumbnailVisualState::Ready) {
        const float prev = m_ThumbFade;
        m_ThumbFade = std::min(1.0f, m_ThumbFade + deltaTime / kImageFadeSeconds);
        if (std::fabs(m_ThumbFade - prev) > 0.001f) {
            thumbDirty = true;
        }
    }

    const float prevHover = m_HoverAnim;
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered && !m_Selected ? 1.0f : 0.0f,
        LMetric(ThemeToken::HoverAnimationDamping));
    if (thumbDirty || std::fabs(m_HoverAnim - prevHover) > 0.001f) {
        InvalidateUI();
    }
    Widget::Tick(deltaTime);
}

// â”€â”€ ProjectListRow â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

ProjectListRow::ProjectListRow(ProjectSummary summary, bool selected, bool favorite)
    : m_Summary(std::move(summary))
    , m_Selected(selected)
    , m_Favorite(favorite) {
    BeginThumbnailLoad(0.05f);
}

void ProjectListRow::BeginThumbnailLoad(float staggerSeconds) {
    m_ThumbState = ThumbnailVisualState::Skeleton;
    m_ThumbFade = 1.0f;
    m_ThumbDelay = staggerSeconds;
    m_ThumbAge = 0.0f;
}

Size ProjectListRow::Measure(const Size& availableSize) {
    const float s = LScale();
    const float h = 64.0f * s;
    m_DesiredSize = Size{ availableSize.width > 0.0f ? availableSize.width : 640.0f * s, h };
    return m_DesiredSize;
}

void ProjectListRow::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

Rect ProjectListRow::MoreRect() const {
    const float s = LScale();
    const float btn = 28.0f * s;
    const float pad = 10.0f * s;
    return Rect{
        m_Geometry.x + m_Geometry.width - pad - btn,
        m_Geometry.y + (m_Geometry.height - btn) * 0.5f,
        btn,
        btn
    };
}

Rect ProjectListRow::OpenRect() const {
    const float s = LScale();
    const float h = 28.0f * s;
    const float w = 64.0f * s;
    const float gap = 6.0f * s;
    const Rect more = MoreRect();
    return Rect{
        more.x - gap - w,
        m_Geometry.y + (m_Geometry.height - h) * 0.5f,
        w,
        h
    };
}

Rect ProjectListRow::FavoriteRect() const {
    const float s = LScale();
    const float btn = 28.0f * s;
    const float gap = 6.0f * s;
    const Rect open = OpenRect();
    return Rect{
        open.x - gap - btn,
        m_Geometry.y + (m_Geometry.height - btn) * 0.5f,
        btn,
        btn
    };
}

ProjectListRow::HitZone ProjectListRow::HitTest(const Point& p) const {
    if (FavoriteRect().Contains(p)) {
        return HitZone::Favorite;
    }
    if (OpenRect().Contains(p)) {
        return HitZone::Open;
    }
    if (MoreRect().Contains(p)) {
        return HitZone::More;
    }
    if (m_Geometry.Contains(p)) {
        return HitZone::Body;
    }
    return HitZone::None;
}

void ProjectListRow::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(ThemeToken::CornerRadiusSmall) * s;

    Color bg = Color::Transparent();
    if (m_Selected) {
        bg = LColor(ThemeToken::SelectedBackground);
    } else if (m_HoverAnim > 0.01f) {
        bg = Color::Lerp(bg, LColor(ThemeToken::HoverBackground), m_HoverAnim);
    }
    if (bg.a > 0.01f) {
        context.DrawRoundedRect(m_Geometry, bg, radius);
    }
    if (m_Selected) {
        context.DrawRoundedRectOutline(m_Geometry, LColor(ThemeToken::AccentPrimary), 1.0f, radius);
    }

    const float pad = 12.0f * s;
    Rect thumb{ m_Geometry.x + pad, m_Geometry.y + (m_Geometry.height - 44.0f * s) * 0.5f, 44.0f * s, 44.0f * s };
    const float thumbRadius = radius;
    if (m_ThumbState == ThumbnailVisualState::Skeleton) {
        PaintSkeletonBone(context, thumb, thumbRadius, 0.1f);
    } else {
        PaintProjectThumbnailPlaceholder(
            context,
            thumb,
            thumbRadius,
            m_Summary.descriptor.displayName,
            m_Summary.descriptor.templateId,
            m_Summary.compatible,
            std::clamp(m_ThumbFade, 0.0f, 1.0f));
    }

    const float titleSize = LMetric(ThemeToken::TextSizeBody) * s;
    const float metaSize = LMetric(ThemeToken::TextSizeCaption) * s;
    const float textX = thumb.x + thumb.width + 12.0f * s;

    context.DrawText(
        m_Summary.descriptor.displayName,
        Point{ textX, m_Geometry.y + 14.0f * s },
        LColor(ThemeToken::TextPrimary),
        titleSize,
        true);

    const std::string meta = m_Summary.descriptor.engineVersion
        + "  Â·  " + FormatRelativeTime(m_Summary.descriptor.lastOpenedUtc)
        + "  Â·  " + EllipsizePath(m_Summary.projectRoot, 36);
    context.DrawText(
        meta,
        Point{ textX, m_Geometry.y + 14.0f * s + titleSize + 4.0f * s },
        LColor(ThemeToken::TextMuted),
        metaSize);

    PaintIconButton(
        context,
        FavoriteRect(),
        Icons::StarName,
        m_HoverZone == HitZone::Favorite,
        m_Favorite,
        radius);

    PaintPrimaryPill(
        context,
        OpenRect(),
        "Open",
        m_HoverZone == HitZone::Open,
        m_Pressed == HitZone::Open);

    PaintIconButton(
        context,
        MoreRect(),
        Icons::MoreName,
        m_HoverZone == HitZone::More,
        false,
        radius);
}

void ProjectListRow::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = HitTest(event.position);
    }
}

void ProjectListRow::OnMouseMove(const MouseEvent& event) {
    const HitZone zone = HitTest(event.position);
    if (zone != m_HoverZone) {
        m_HoverZone = zone;
        InvalidateUI();
    }
    m_Hovered = zone != HitZone::None;
}

void ProjectListRow::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left || m_Pressed == HitZone::None) {
        m_Pressed = HitZone::None;
        return;
    }
    const HitZone hit = HitTest(event.position);
    if (hit == m_Pressed) {
        if (hit == HitZone::Body && m_OnSelect) {
            m_OnSelect();
        } else if (hit == HitZone::Favorite) {
            if (m_OnAction) {
                m_OnAction(ProjectCardAction::Favorite);
            }
        } else if (hit == HitZone::Open) {
            if (m_OnSelect) {
                m_OnSelect();
            }
            if (m_OnAction) {
                m_OnAction(ProjectCardAction::Open);
            }
        } else if (hit == HitZone::More) {
            if (m_OnSelect) {
                m_OnSelect();
            }
            if (m_OnAction) {
                m_OnAction(ProjectCardAction::More);
            }
        }
    }
    m_Pressed = HitZone::None;
}

bool ProjectListRow::ShowsPointerCursor(const Point& position) const {
    return HitTest(position) != HitZone::None;
}

void ProjectListRow::Tick(float deltaTime) {
    AdvanceSkeletonShimmer(deltaTime);

    m_ThumbAge += deltaTime;
    bool thumbDirty = false;
    if (m_ThumbState == ThumbnailVisualState::Skeleton) {
        if (m_ThumbAge >= m_ThumbDelay) {
            m_ThumbState = ThumbnailVisualState::Placeholder;
            m_ThumbFade = 0.0f;
            thumbDirty = true;
        } else {
            thumbDirty = true;
        }
    } else {
        const float prev = m_ThumbFade;
        m_ThumbFade = std::min(1.0f, m_ThumbFade + deltaTime / ProjectCard::kImageFadeSeconds);
        if (std::fabs(m_ThumbFade - prev) > 0.001f) {
            thumbDirty = true;
        }
    }

    const float prevHover = m_HoverAnim;
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered && !m_Selected ? 1.0f : 0.0f,
        LMetric(ThemeToken::HoverAnimationDamping));
    if (thumbDirty || std::fabs(m_HoverAnim - prevHover) > 0.001f) {
        InvalidateUI();
    }
    Widget::Tick(deltaTime);
}

// â”€â”€ ProjectGrid â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void ProjectGrid::SetCards(std::vector<std::shared_ptr<ProjectCard>> cards) {
    ClearChildren();
    m_Cards = std::move(cards);
    for (auto& card : m_Cards) {
        if (card) {
            AddChild(card);
        }
    }
}

Size ProjectGrid::Measure(const Size& availableSize) {
    const float s = LScale();
    const float gap = m_Gap * s;
    const float cardW = ProjectCard::kCardWidth * s;
    const float cardH = ProjectCard::kCardHeight * s;
    const float width = availableSize.width > 0.0f ? availableSize.width : cardW;

    m_Columns = std::max(1, static_cast<int>(std::floor((width + gap) / (cardW + gap))));
    const int count = static_cast<int>(m_Cards.size());
    const int rows = count == 0 ? 0 : (count + m_Columns - 1) / m_Columns;
    const float height = rows > 0
        ? static_cast<float>(rows) * cardH + static_cast<float>(rows - 1) * gap
        : 80.0f * s;

    for (auto& card : m_Cards) {
        if (card) {
            card->Measure(Size{ cardW, cardH });
        }
    }

    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void ProjectGrid::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float s = LScale();
    const float gap = m_Gap * s;
    const float cardW = ProjectCard::kCardWidth * s;
    const float cardH = ProjectCard::kCardHeight * s;

    m_Columns = std::max(1, static_cast<int>(std::floor((allottedRect.width + gap) / (cardW + gap))));

    for (int i = 0; i < static_cast<int>(m_Cards.size()); ++i) {
        if (!m_Cards[static_cast<std::size_t>(i)]) {
            continue;
        }
        const int col = i % m_Columns;
        const int row = i / m_Columns;
        m_Cards[static_cast<std::size_t>(i)]->Arrange(Rect{
            allottedRect.x + static_cast<float>(col) * (cardW + gap),
            allottedRect.y + static_cast<float>(row) * (cardH + gap),
            cardW,
            cardH
        });
    }
}

void ProjectGrid::Paint(PaintContext& context) {
    for (auto& card : m_Cards) {
        if (card && card->IsVisible()) {
            card->Paint(context);
        }
    }
}

void ProjectGrid::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
}

// â”€â”€ TemplateCard â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TemplateCard::TemplateCard(ProjectTemplateInfo info, bool selected)
    : m_Info(std::move(info))
    , m_Selected(selected) {
    BeginThumbnailLoad(0.1f);
}

void TemplateCard::SetThumbnail(
    we::rhi::RHIDescriptorSetHandle texture,
    ThumbnailVisualState state) {
    m_ThumbTexture = texture;
    m_ThumbState = state;
    m_ThumbDelay = 0.0f;
    m_ThumbFade = (state == ThumbnailVisualState::Skeleton) ? 1.0f : 0.0f;
    InvalidateUI();
}

void TemplateCard::BeginThumbnailLoad(float staggerSeconds) {
    m_ThumbState = ThumbnailVisualState::Skeleton;
    m_ThumbTexture = we::rhi::RHIDescriptorSetHandle::Invalid;
    m_ThumbFade = 1.0f;
    m_ThumbDelay = staggerSeconds;
    m_ThumbAge = 0.0f;
}

Size TemplateCard::Measure(const Size& availableSize) {
    (void)availableSize;
    const float s = LScale();
    m_DesiredSize = Size{ kWidth * s, kHeight * s };
    return m_DesiredSize;
}

void TemplateCard::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TemplateCard::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(ThemeToken::CornerRadiusMedium) * s;

    Color bg = LColor(ThemeToken::PanelBackground);
    if (m_Selected) {
        bg = LColor(ThemeToken::SelectedBackground);
    } else if (m_HoverAnim > 0.01f) {
        bg = Color::Lerp(bg, LColor(ThemeToken::HoverBackground), m_HoverAnim * 0.55f);
    }
    context.DrawRoundedRect(m_Geometry, bg, radius);
    context.DrawRoundedRectOutline(
        m_Geometry,
        m_Selected ? LColor(ThemeToken::AccentPrimary) : LColor(ThemeToken::BorderDefault),
        m_Selected ? 1.5f : 1.0f,
        radius);

    const float pad = LMetric(ThemeToken::Space3) * s;
    Rect thumb{ m_Geometry.x + pad, m_Geometry.y + pad, m_Geometry.width - pad * 2.0f, 96.0f * s };
    if (m_ThumbState == ThumbnailVisualState::Skeleton) {
        PaintSkeletonBone(context, thumb, radius - 2.0f, 0.2f);
    } else if (m_ThumbState == ThumbnailVisualState::Ready
        && m_ThumbTexture != we::rhi::RHIDescriptorSetHandle::Invalid) {
        PaintTemplateThumbnailPlaceholder(
            context,
            thumb,
            radius - 2.0f,
            m_Info.displayName,
            m_Info.category,
            m_Info.id,
            1.0f);
        Color tint = Color::White();
        tint.a = std::clamp(m_ThumbFade, 0.0f, 1.0f);
        context.DrawTexture(thumb, m_ThumbTexture, tint);
    } else {
        PaintTemplateThumbnailPlaceholder(
            context,
            thumb,
            radius - 2.0f,
            m_Info.displayName,
            m_Info.category,
            m_Info.id,
            std::clamp(m_ThumbFade, 0.0f, 1.0f));
    }

    const float titleSize = LMetric(ThemeToken::TextSizeBody) * s;
    const float metaSize = LMetric(ThemeToken::TextSizeCaption) * s;
    float y = thumb.y + thumb.height + 10.0f * s;
    context.DrawText(
        m_Info.displayName,
        Point{ m_Geometry.x + pad, y },
        LColor(ThemeToken::TextPrimary),
        titleSize,
        true);
    y += titleSize + 4.0f * s;

    std::string desc = m_Info.description;
    if (desc.size() > 64) {
        desc = desc.substr(0, 61) + "...";
    }
    context.DrawText(desc, Point{ m_Geometry.x + pad, y }, LColor(ThemeToken::TextMuted), metaSize);

    if (!m_Info.platforms.empty()) {
        y += metaSize + 6.0f * s;
        std::string platforms;
        for (std::size_t i = 0; i < m_Info.platforms.size() && i < 4; ++i) {
            if (i > 0) {
                platforms += "  ";
            }
            platforms += m_Info.platforms[i];
        }
        context.DrawText(platforms, Point{ m_Geometry.x + pad, y }, LColor(ThemeToken::AccentPrimary), metaSize);
    }
}

void TemplateCard::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Geometry.Contains(event.position)) {
        m_Pressed = true;
    }
}

void TemplateCard::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed && m_Geometry.Contains(event.position)) {
        if (m_OnSelect) {
            m_OnSelect();
        }
    }
    m_Pressed = false;
}

bool TemplateCard::ShowsPointerCursor(const Point& position) const {
    return m_Geometry.Contains(position);
}

void TemplateCard::Tick(float deltaTime) {
    AdvanceSkeletonShimmer(deltaTime);

    m_ThumbAge += deltaTime;
    bool thumbDirty = false;
    if (m_ThumbState == ThumbnailVisualState::Skeleton) {
        if (m_ThumbAge >= m_ThumbDelay) {
            m_ThumbState = ThumbnailVisualState::Placeholder;
            m_ThumbFade = 0.0f;
            thumbDirty = true;
        } else {
            thumbDirty = true;
        }
    } else {
        const float prev = m_ThumbFade;
        m_ThumbFade = std::min(1.0f, m_ThumbFade + deltaTime / kImageFadeSeconds);
        if (std::fabs(m_ThumbFade - prev) > 0.001f) {
            thumbDirty = true;
        }
    }

    const float prevHover = m_HoverAnim;
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered && !m_Selected ? 1.0f : 0.0f,
        LMetric(ThemeToken::HoverAnimationDamping));
    if (thumbDirty || std::fabs(m_HoverAnim - prevHover) > 0.001f) {
        InvalidateUI();
    }
    Widget::Tick(deltaTime);
}

// â”€â”€ DashboardTile â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

DashboardTile::DashboardTile(std::string title, std::string body, const char* icon)
    : m_Title(std::move(title))
    , m_Body(std::move(body))
    , m_Icon(icon) {
}

Size DashboardTile::Measure(const Size& availableSize) {
    const float s = LScale();
    const float pad = LMetric(ThemeToken::Space3) * s;
    const float titleSize = LMetric(ThemeToken::TextSizeBody) * s;
    const float bodySize = LMetric(ThemeToken::TextSizeCaption) * s;
    const float gap = LMetric(ThemeToken::Space2) * s;
    const float iconSize = (m_Icon && m_Icon[0]) ? 20.0f * s : 0.0f;
    const float iconGap = iconSize > 0.0f ? LMetric(ThemeToken::Space2) * s : 0.0f;

    const float width = availableSize.width > 0.0f ? availableSize.width : 280.0f * s;
    const float textWidth = std::max(40.0f * s, width - pad * 2.0f - iconSize - iconGap);
    const int bodyLines = EstimateWrappedLines(m_Body, bodySize, textWidth);
    const float textBlockH = titleSize + gap + static_cast<float>(bodyLines) * (bodySize + 2.0f * s);
    const float contentH = pad * 2.0f + std::max(iconSize, textBlockH);

    m_DesiredSize = Size{ width, std::max(kMinHeight * s, contentH) };
    return m_DesiredSize;
}

void DashboardTile::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void DashboardTile::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(ThemeToken::CornerRadiusMedium) * s;

    Color bg = LColor(ThemeToken::PanelBackground);
    if (m_HoverAnim > 0.01f) {
        bg = Color::Lerp(bg, LColor(ThemeToken::HoverBackground), m_HoverAnim * 0.55f);
    }
    if (m_Pressed) {
        bg = Color::Lerp(bg, LColor(ThemeToken::SelectedBackground), 0.35f);
    }
    context.DrawRoundedRect(m_Geometry, bg, radius);

    Color border = m_Accent ? LColor(ThemeToken::AccentPrimary) : LColor(ThemeToken::BorderDefault);
    context.DrawRoundedRectOutline(m_Geometry, border, m_Accent ? 1.5f : 1.0f, radius);

    const float pad = LMetric(ThemeToken::Space3) * s;
    const float titleSize = LMetric(ThemeToken::TextSizeBody) * s;
    const float bodySize = LMetric(ThemeToken::TextSizeCaption) * s;
    const float gap = LMetric(ThemeToken::Space2) * s;
    float x = m_Geometry.x + pad;
    float y = m_Geometry.y + pad;

    if (m_Icon && m_Icon[0]) {
        const float iconSize = 20.0f * s;
        IconPainter::DrawIcon(
            context,
            m_Icon,
            Rect{ x, y, iconSize, iconSize },
            m_Accent ? LColor(ThemeToken::AccentPrimary) : LColor(ThemeToken::IconAccent));
        x += iconSize + LMetric(ThemeToken::Space2) * s;
    }

    context.DrawText(m_Title, Point{ x, y }, LColor(ThemeToken::TextPrimary), titleSize, true);
    y += titleSize + gap;

    // Simple wrap for body text within remaining width.
    const float maxW = m_Geometry.x + m_Geometry.width - pad - x;
    const float charW = std::max(1.0f, bodySize * 0.55f);
    const int charsPerLine = std::max(8, static_cast<int>(maxW / charW));
    std::size_t offset = 0;
    while (offset < m_Body.size() && y + bodySize <= m_Geometry.y + m_Geometry.height - pad) {
        const std::size_t take = std::min(static_cast<std::size_t>(charsPerLine), m_Body.size() - offset);
        context.DrawText(
            m_Body.substr(offset, take),
            Point{ x, y },
            LColor(ThemeToken::TextMuted),
            bodySize);
        offset += take;
        y += bodySize + 2.0f * s;
    }
}

void DashboardTile::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Geometry.Contains(event.position) && m_OnClick) {
        m_Pressed = true;
        InvalidateUI();
    }
}

void DashboardTile::OnMouseMove(const MouseEvent& event) {
    const bool hovered = m_Geometry.Contains(event.position);
    if (hovered != m_Hovered) {
        m_Hovered = hovered;
        InvalidateUI();
    }
}

void DashboardTile::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed && m_Geometry.Contains(event.position)) {
        if (m_OnClick) {
            m_OnClick();
        }
    }
    if (m_Pressed) {
        m_Pressed = false;
        InvalidateUI();
    }
}

bool DashboardTile::ShowsPointerCursor(const Point& position) const {
    return m_OnClick && m_Geometry.Contains(position);
}

void DashboardTile::Tick(float deltaTime) {
    (void)deltaTime;
    const float prev = m_HoverAnim;
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered ? 1.0f : 0.0f,
        LMetric(ThemeToken::HoverAnimationDamping));
    if (std::fabs(m_HoverAnim - prev) > 0.001f) {
        InvalidateUI();
    }
    Widget::Tick(deltaTime);
}

} // namespace we::programs::welauncher
