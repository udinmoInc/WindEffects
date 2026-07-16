#pragma once

#include "Core/Widget.h"

#include <functional>
#include <memory>
#include <vector>

namespace we::programs::welauncher {

// Shared shimmer phase for coordinated skeleton animations.
[[nodiscard]] float SkeletonShimmerPhase();
void AdvanceSkeletonShimmer(float deltaTime);

void PaintSkeletonBone(
    WindEffects::Editor::UI::PaintContext& context,
    const WindEffects::Editor::UI::Rect& rect,
    float radius,
    float phaseOffset = 0.0f);

enum class SkeletonKind {
    ProjectCard,
    TemplateCard,
    EngineCard,
    NewsCard,
    ListRow,
    DashboardTile
};

// Reusable shimmering skeleton placeholder for dynamic launcher content.
class SkeletonCard : public WindEffects::Editor::UI::Widget {
public:
    explicit SkeletonCard(SkeletonKind kind = SkeletonKind::ProjectCard);

    void SetKind(SkeletonKind kind) { m_Kind = kind; }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    SkeletonKind m_Kind = SkeletonKind::ProjectCard;
};

class SkeletonGrid : public WindEffects::Editor::UI::Widget {
public:
    void SetCount(int count, SkeletonKind kind = SkeletonKind::ProjectCard);
    void SetGap(float gap) { m_Gap = gap; }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    std::vector<std::shared_ptr<SkeletonCard>> m_Cards;
    SkeletonKind m_Kind = SkeletonKind::ProjectCard;
    float m_Gap = 14.0f;
    int m_Columns = 1;
};

} // namespace we::programs::welauncher
