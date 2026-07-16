#pragma once

#include "Core/Widget.h"
#include "Model/WeProjectDescriptor.h"
#include "UI/Widgets/LauncherControls.h"
#include "UI/Widgets/ProjectViews.h"

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace we::programs::welauncher {

// Productivity-first project table: Favorite | Name | Last Opened | Engine | Platform | Status.
struct ProjectColumnLayout {
    float favorite = 36.0f;
    float name = 280.0f;
    float lastOpened = 110.0f;
    float engine = 120.0f;
    float platform = 100.0f;
    float status = 100.0f;
    float more = 32.0f;

    static ProjectColumnLayout Compute(float totalWidth, float scale);
};

class ProjectTableHeader : public WindEffects::Editor::UI::Widget {
public:
    using SortFn = std::function<void(ProjectSortMode)>;

    void SetSortMode(ProjectSortMode mode) { m_SortMode = mode; }
    void SetOnSort(SortFn fn) { m_OnSort = std::move(fn); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;

    static constexpr float kHeight = 30.0f;

private:
    ProjectSortMode m_SortMode = ProjectSortMode::Recent;
    SortFn m_OnSort;
    int m_PressedCol = -1;
};

// Compact desktop table row (44–48 px). Single-line cells; context menu via ⋮ / right-click.
class ProjectTableRow : public WindEffects::Editor::UI::Widget {
public:
    using ActionFn = std::function<void(ProjectCardAction)>;
    using SelectFn = std::function<void(bool additive)>;

    ProjectTableRow(ProjectSummary summary, bool selected, bool favorite = false);

    void SetSelected(bool selected) { m_Selected = selected; }
    void SetFavorite(bool favorite) { m_Favorite = favorite; }
    void SetOnAction(ActionFn fn) { m_OnAction = std::move(fn); }
    void SetOnSelect(SelectFn fn) { m_OnSelect = std::move(fn); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kRowH = 48.0f;

private:
    enum class HitZone { None, Body, Favorite, More };

    [[nodiscard]] HitZone HitTest(const WindEffects::Editor::UI::Point& p) const;
    [[nodiscard]] WindEffects::Editor::UI::Rect FavoriteRect() const;
    [[nodiscard]] WindEffects::Editor::UI::Rect MoreRect() const;

    ProjectSummary m_Summary;
    bool m_Selected = false;
    bool m_Favorite = false;
    ActionFn m_OnAction;
    SelectFn m_OnSelect;
    HitZone m_Pressed = HitZone::None;
    HitZone m_HoverZone = HitZone::None;
    float m_HoverAnim = 0.0f;
    bool m_CtrlDown = false;
    std::chrono::steady_clock::time_point m_LastClick{};
};

class TemplateListRow : public WindEffects::Editor::UI::Widget {
public:
    using SelectFn = std::function<void()>;

    TemplateListRow(ProjectTemplateInfo info, bool selected);

    void SetSelected(bool selected) { m_Selected = selected; }
    void SetOnSelect(SelectFn fn) { m_OnSelect = std::move(fn); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kRowH = 52.0f;

private:
    ProjectTemplateInfo m_Info;
    bool m_Selected = false;
    bool m_Pressed = false;
    SelectFn m_OnSelect;
    float m_HoverAnim = 0.0f;
};

class EngineInstallRow : public WindEffects::Editor::UI::Widget {
public:
    using ActionFn = std::function<void(const std::string& action)>;

    EngineInstallRow(EngineInstallInfo info, bool selected);

    void SetSelected(bool selected) { m_Selected = selected; }
    void SetOnSelect(std::function<void()> fn) { m_OnSelect = std::move(fn); }
    void SetOnAction(ActionFn fn) { m_OnAction = std::move(fn); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kRowH = 48.0f;

private:
    enum class HitZone { None, Body, Launch, Verify, Repair, Folder, Uninstall };

    [[nodiscard]] HitZone HitTest(const WindEffects::Editor::UI::Point& p) const;
    [[nodiscard]] WindEffects::Editor::UI::Rect ActionRect(int index) const;

    EngineInstallInfo m_Info;
    bool m_Selected = false;
    std::function<void()> m_OnSelect;
    ActionFn m_OnAction;
    HitZone m_Pressed = HitZone::None;
    HitZone m_HoverZone = HitZone::None;
    float m_HoverAnim = 0.0f;
};

class LibraryPackageRow : public WindEffects::Editor::UI::Widget {
public:
    using ClickFn = std::function<void()>;

    LibraryPackageRow(
        std::string kind,
        std::string name,
        std::string detail,
        const char* icon,
        ClickFn onClick = {});

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kRowH = 44.0f;

private:
    std::string m_Kind;
    std::string m_Name;
    std::string m_Detail;
    const char* m_Icon = nullptr;
    ClickFn m_OnClick;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
};

[[nodiscard]] const char* TemplateTypeIcon(const std::string& templateId);
[[nodiscard]] WindEffects::Editor::UI::Color TemplateBadgeColor(const std::string& templateId);

} // namespace we::programs::welauncher
