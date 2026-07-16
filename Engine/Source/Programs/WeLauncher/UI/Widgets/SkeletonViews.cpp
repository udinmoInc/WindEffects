#include "UI/Widgets/SkeletonViews.h"

#include "UI/LauncherHelpers.h"

#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace WindEffects::Editor::UI;

namespace we::programs::welauncher {
namespace {

float g_ShimmerPhase = 0.0f;

Size CardSizeForKind(SkeletonKind kind, float scale) {
    switch (kind) {
    case SkeletonKind::TemplateCard:
        return Size{ 260.0f * scale, 210.0f * scale };
    case SkeletonKind::EngineCard:
        return Size{ 0.0f, 168.0f * scale }; // width fills
    case SkeletonKind::NewsCard:
        return Size{ 0.0f, 110.0f * scale };
    case SkeletonKind::ListRow:
        return Size{ 0.0f, 56.0f * scale };
    case SkeletonKind::DashboardTile:
        return Size{ 0.0f, 88.0f * scale };
    case SkeletonKind::ProjectCard:
    default:
        return Size{ 236.0f * scale, 236.0f * scale };
    }
}

} // namespace

float SkeletonShimmerPhase() {
    return g_ShimmerPhase;
}

void AdvanceSkeletonShimmer(float deltaTime) {
    g_ShimmerPhase += deltaTime * 1.15f;
    if (g_ShimmerPhase > 1000.0f) {
        g_ShimmerPhase = std::fmod(g_ShimmerPhase, 1.0f);
    }
}

void PaintSkeletonBone(PaintContext& context, const Rect& rect, float radius, float phaseOffset) {
    if (rect.width <= 1.0f || rect.height <= 1.0f) {
        return;
    }

    Color base = LColor(ThemeToken::PanelContentBackground);
    Color peak = LColor(ThemeToken::HoverBackground);
    peak = Color::Lerp(base, peak, 0.55f);

    context.DrawRoundedRect(rect, base, radius);

    // Soft shimmer sweep across the bone.
    const float phase = SkeletonShimmerPhase() + phaseOffset;
    const float sweep = std::fmod(phase * 0.55f, 1.0f);
    const float bandW = std::max(24.0f, rect.width * 0.35f);
    const float bandX = rect.x - bandW + sweep * (rect.width + bandW * 2.0f);

    Color glow = peak;
    glow.a = 0.22f + 0.18f * (0.5f + 0.5f * std::sin(phase * 6.2831853f));
    const float clipL = std::max(rect.x, bandX);
    const float clipR = std::min(rect.x + rect.width, bandX + bandW);
    if (clipR > clipL) {
        context.DrawRoundedRect(
            Rect{ clipL, rect.y, clipR - clipL, rect.height },
            glow,
            radius);
    }
}

SkeletonCard::SkeletonCard(SkeletonKind kind)
    : m_Kind(kind) {
}

Size SkeletonCard::Measure(const Size& availableSize) {
    const float s = LScale();
    Size size = CardSizeForKind(m_Kind, s);
    if (size.width <= 0.0f) {
        size.width = availableSize.width > 0.0f ? availableSize.width : 320.0f * s;
    }
    m_DesiredSize = size;
    return m_DesiredSize;
}

void SkeletonCard::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void SkeletonCard::Paint(PaintContext& context) {
    const float s = LScale();
    const float radius = LMetric(ThemeToken::CornerRadiusMedium) * s;
    const float pad = LMetric(ThemeToken::Space3) * s;
    const float boneR = LMetric(ThemeToken::CornerRadiusSmall) * s;

    Color cardBg = LColor(ThemeToken::PanelBackground);
    context.DrawRoundedRect(m_Geometry, cardBg, radius);
    context.DrawRoundedRectOutline(m_Geometry, LColor(ThemeToken::BorderDefault), 1.0f, radius);

    const float phase = static_cast<float>(reinterpret_cast<uintptr_t>(this) % 97) * 0.07f;

    switch (m_Kind) {
    case SkeletonKind::ProjectCard: {
        Rect thumb{
            m_Geometry.x + pad,
            m_Geometry.y + pad,
            m_Geometry.width - pad * 2.0f,
            112.0f * s
        };
        PaintSkeletonBone(context, thumb, radius - 2.0f, phase);

        float y = thumb.y + thumb.height + 12.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.62f, 14.0f * s },
            boneR,
            phase + 0.2f);
        y += 20.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.48f, 10.0f * s },
            boneR,
            phase + 0.35f);
        y += 16.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.72f, 10.0f * s },
            boneR,
            phase + 0.5f);

        const float btnH = 28.0f * s;
        PaintSkeletonBone(
            context,
            Rect{
                m_Geometry.x + pad,
                m_Geometry.y + m_Geometry.height - pad - btnH,
                72.0f * s,
                btnH
            },
            btnH * 0.5f,
            phase + 0.65f);
        PaintSkeletonBone(
            context,
            Rect{
                m_Geometry.x + m_Geometry.width - pad - 28.0f * s,
                m_Geometry.y + m_Geometry.height - pad - btnH,
                28.0f * s,
                btnH
            },
            boneR,
            phase + 0.8f);
        break;
    }
    case SkeletonKind::TemplateCard: {
        Rect thumb{
            m_Geometry.x + pad,
            m_Geometry.y + pad,
            m_Geometry.width - pad * 2.0f,
            96.0f * s
        };
        PaintSkeletonBone(context, thumb, radius - 2.0f, phase);
        float y = thumb.y + thumb.height + 12.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.55f, 14.0f * s },
            boneR,
            phase + 0.25f);
        y += 22.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.85f, 10.0f * s },
            boneR,
            phase + 0.4f);
        y += 16.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.4f, 10.0f * s },
            boneR,
            phase + 0.55f);
        break;
    }
    case SkeletonKind::EngineCard: {
        float y = m_Geometry.y + pad;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.4f, 16.0f * s },
            boneR,
            phase);
        y += 28.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.78f, 12.0f * s },
            boneR,
            phase + 0.2f);
        y += 22.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, m_Geometry.width * 0.55f, 12.0f * s },
            boneR,
            phase + 0.35f);
        y += 22.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, y, 100.0f * s, 28.0f * s },
            14.0f * s,
            phase + 0.5f);
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad + 112.0f * s, y, 88.0f * s, 28.0f * s },
            14.0f * s,
            phase + 0.65f);
        break;
    }
    case SkeletonKind::NewsCard:
    case SkeletonKind::DashboardTile: {
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, m_Geometry.y + pad, 28.0f * s, 28.0f * s },
            boneR,
            phase);
        float y = m_Geometry.y + pad;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad + 40.0f * s, y, m_Geometry.width * 0.45f, 14.0f * s },
            boneR,
            phase + 0.2f);
        y += 24.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad + 40.0f * s, y, m_Geometry.width * 0.7f, 10.0f * s },
            boneR,
            phase + 0.35f);
        y += 16.0f * s;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad + 40.0f * s, y, m_Geometry.width * 0.55f, 10.0f * s },
            boneR,
            phase + 0.5f);
        break;
    }
    case SkeletonKind::ListRow: {
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad, m_Geometry.y + (m_Geometry.height - 36.0f * s) * 0.5f, 36.0f * s, 36.0f * s },
            boneR,
            phase);
        const float textY = m_Geometry.y + (m_Geometry.height - 14.0f * s) * 0.35f;
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad + 48.0f * s, textY, m_Geometry.width * 0.35f, 14.0f * s },
            boneR,
            phase + 0.2f);
        PaintSkeletonBone(
            context,
            Rect{ m_Geometry.x + pad + 48.0f * s, textY + 18.0f * s, m_Geometry.width * 0.55f, 10.0f * s },
            boneR,
            phase + 0.4f);
        break;
    }
    }
}

void SkeletonCard::Tick(float deltaTime) {
    AdvanceSkeletonShimmer(deltaTime);
    InvalidateUI();
    Widget::Tick(deltaTime);
}

void SkeletonGrid::SetCount(int count, SkeletonKind kind) {
    ClearChildren();
    m_Kind = kind;
    m_Cards.clear();
    m_Cards.reserve(static_cast<std::size_t>(std::max(0, count)));
    for (int i = 0; i < count; ++i) {
        auto card = std::make_shared<SkeletonCard>(kind);
        m_Cards.push_back(card);
        AddChild(card);
    }
}

Size SkeletonGrid::Measure(const Size& availableSize) {
    const float s = LScale();
    const float gap = m_Gap * s;
    const Size cardSize = CardSizeForKind(m_Kind, s);
    const float cardW = cardSize.width > 0.0f ? cardSize.width : 236.0f * s;
    const float cardH = cardSize.height;
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

void SkeletonGrid::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float s = LScale();
    const float gap = m_Gap * s;
    const Size cardSize = CardSizeForKind(m_Kind, s);
    const float cardW = cardSize.width > 0.0f ? cardSize.width : 236.0f * s;
    const float cardH = cardSize.height;
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

void SkeletonGrid::Paint(PaintContext& context) {
    for (auto& card : m_Cards) {
        if (card && card->IsVisible()) {
            card->Paint(context);
        }
    }
}

void SkeletonGrid::Tick(float deltaTime) {
    AdvanceSkeletonShimmer(deltaTime);
    Widget::Tick(deltaTime);
    InvalidateUI();
}

} // namespace we::programs::welauncher
