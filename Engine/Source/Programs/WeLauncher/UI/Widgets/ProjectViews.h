#pragma once

#include "Core/Widget.h"
#include "Model/WeProjectDescriptor.h"
#include "RHI/Types.h"
#include "UI/ThumbnailPlaceholders.h"

#include <functional>
#include <string>
#include <vector>

namespace we::programs::welauncher {

enum class ProjectCardAction {
    Open,
    Clone,
    Rename,
    Delete,
    ShowInExplorer,
    Favorite,
    More
};

class ProjectCard : public WindEffects::Editor::UI::Widget {
public:
    using ActionFn = std::function<void(ProjectCardAction)>;
    using SelectFn = std::function<void()>;

    ProjectCard(ProjectSummary summary, bool selected, bool favorite = false);

    void SetSelected(bool selected) { m_Selected = selected; }
    void SetFavorite(bool favorite) { m_Favorite = favorite; }
    void SetOnAction(ActionFn fn) { m_OnAction = std::move(fn); }
    void SetOnSelect(SelectFn fn) { m_OnSelect = std::move(fn); }
    void SetThumbnail(
        we::rhi::RHIDescriptorSetHandle texture,
        ThumbnailVisualState state = ThumbnailVisualState::Ready);
    void BeginThumbnailLoad(float staggerSeconds = 0.0f);
    [[nodiscard]] const ProjectSummary& Summary() const { return m_Summary; }
    [[nodiscard]] bool IsFavorite() const { return m_Favorite; }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kCardWidth = 236.0f;
    static constexpr float kCardHeight = 236.0f;
    static constexpr float kImageFadeSeconds = 0.2f;

private:
    enum class HitZone { None, Body, Favorite, Open, More };

    HitZone HitTest(const WindEffects::Editor::UI::Point& p) const;
    WindEffects::Editor::UI::Rect FavoriteRect() const;
    WindEffects::Editor::UI::Rect OpenRect() const;
    WindEffects::Editor::UI::Rect MoreRect() const;
    void PaintThumb(WindEffects::Editor::UI::PaintContext& context, const WindEffects::Editor::UI::Rect& thumb, float radius);

    ProjectSummary m_Summary;
    bool m_Selected = false;
    bool m_Favorite = false;
    ActionFn m_OnAction;
    SelectFn m_OnSelect;
    HitZone m_Pressed = HitZone::None;
    HitZone m_HoverZone = HitZone::None;
    float m_HoverAnim = 0.0f;

    ThumbnailVisualState m_ThumbState = ThumbnailVisualState::Skeleton;
    we::rhi::RHIDescriptorSetHandle m_ThumbTexture = we::rhi::RHIDescriptorSetHandle::Invalid;
    float m_ThumbFade = 0.0f;
    float m_ThumbDelay = 0.0f;
    float m_ThumbAge = 0.0f;
};

class ProjectListRow : public WindEffects::Editor::UI::Widget {
public:
    using ActionFn = std::function<void(ProjectCardAction)>;
    using SelectFn = std::function<void()>;

    ProjectListRow(ProjectSummary summary, bool selected, bool favorite = false);

    void SetSelected(bool selected) { m_Selected = selected; }
    void SetFavorite(bool favorite) { m_Favorite = favorite; }
    void SetOnAction(ActionFn fn) { m_OnAction = std::move(fn); }
    void SetOnSelect(SelectFn fn) { m_OnSelect = std::move(fn); }
    void BeginThumbnailLoad(float staggerSeconds = 0.0f);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    enum class HitZone { None, Body, Favorite, Open, More };

    HitZone HitTest(const WindEffects::Editor::UI::Point& p) const;
    WindEffects::Editor::UI::Rect FavoriteRect() const;
    WindEffects::Editor::UI::Rect OpenRect() const;
    WindEffects::Editor::UI::Rect MoreRect() const;

    ProjectSummary m_Summary;
    bool m_Selected = false;
    bool m_Favorite = false;
    ActionFn m_OnAction;
    SelectFn m_OnSelect;
    HitZone m_Pressed = HitZone::None;
    HitZone m_HoverZone = HitZone::None;
    float m_HoverAnim = 0.0f;
    ThumbnailVisualState m_ThumbState = ThumbnailVisualState::Skeleton;
    float m_ThumbFade = 0.0f;
    float m_ThumbDelay = 0.0f;
    float m_ThumbAge = 0.0f;
};

class ProjectGrid : public WindEffects::Editor::UI::Widget {
public:
    void SetCards(std::vector<std::shared_ptr<ProjectCard>> cards);
    void SetGap(float gap) { m_Gap = gap; }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    std::vector<std::shared_ptr<ProjectCard>> m_Cards;
    float m_Gap = 14.0f;
    int m_Columns = 1;
};

class TemplateCard : public WindEffects::Editor::UI::Widget {
public:
    using SelectFn = std::function<void()>;

    TemplateCard(ProjectTemplateInfo info, bool selected);

    void SetSelected(bool selected) { m_Selected = selected; }
    void SetOnSelect(SelectFn fn) { m_OnSelect = std::move(fn); }
    void SetThumbnail(
        we::rhi::RHIDescriptorSetHandle texture,
        ThumbnailVisualState state = ThumbnailVisualState::Ready);
    void BeginThumbnailLoad(float staggerSeconds = 0.0f);
    [[nodiscard]] const ProjectTemplateInfo& Info() const { return m_Info; }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kWidth = 260.0f;
    static constexpr float kHeight = 210.0f;
    static constexpr float kImageFadeSeconds = 0.2f;

private:
    ProjectTemplateInfo m_Info;
    bool m_Selected = false;
    bool m_Pressed = false;
    SelectFn m_OnSelect;
    float m_HoverAnim = 0.0f;
    ThumbnailVisualState m_ThumbState = ThumbnailVisualState::Skeleton;
    we::rhi::RHIDescriptorSetHandle m_ThumbTexture = we::rhi::RHIDescriptorSetHandle::Invalid;
    float m_ThumbFade = 0.0f;
    float m_ThumbDelay = 0.0f;
    float m_ThumbAge = 0.0f;
};

// Compact dashboard tile used for Quick Actions / status / news sections.
class DashboardTile : public WindEffects::Editor::UI::Widget {
public:
    using ClickFn = std::function<void()>;

    DashboardTile(std::string title, std::string body, const char* icon = nullptr);

    void SetOnClick(ClickFn fn) { m_OnClick = std::move(fn); }
    void SetAccent(bool accent) { m_Accent = accent; }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kMinHeight = 88.0f;

private:
    std::string m_Title;
    std::string m_Body;
    const char* m_Icon = nullptr;
    ClickFn m_OnClick;
    bool m_Accent = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
};

} // namespace we::programs::welauncher
