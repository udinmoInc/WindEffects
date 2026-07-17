#pragma once

#include "KindUI/Core/Widget.h"
#include "Model/WeProjectDescriptor.h"

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace we::programs::welauncher {

// Fixed-size dialog shell: shadow + 12px rounded graphite card.
class WizardDialogShell : public we::runtime::kindui::Widget {
public:
    WizardDialogShell();

    void SetContent(const std::shared_ptr<we::runtime::kindui::Widget>& content);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

    static constexpr float kWidth = 1100.0f;
    static constexpr float kHeight = 700.0f;
    static constexpr float kRadius = 12.0f;

private:
    std::shared_ptr<we::runtime::kindui::Widget> m_Content;
};

class FilterChip : public we::runtime::kindui::Widget {
public:
    using ClickFn = std::function<void()>;

    FilterChip(std::string label, bool selected);

    void SetSelected(bool selected) { m_Selected = selected; }
    void SetOnClicked(ClickFn fn) { m_OnClicked = std::move(fn); }

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kHeight = 34.0f;

private:
    std::string m_Label;
    bool m_Selected = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    ClickFn m_OnClicked;
};

// 64 px template row for the create wizard list.
class CreateTemplateRow : public we::runtime::kindui::Widget {
public:
    using SelectFn = std::function<void()>;
    using ActivateFn = std::function<void()>;

    CreateTemplateRow(ProjectTemplateInfo info, bool selected);

    void SetSelected(bool selected) { m_Selected = selected; }
    void SetOnSelect(SelectFn fn) { m_OnSelect = std::move(fn); }
    void SetOnActivate(ActivateFn fn) { m_OnActivate = std::move(fn); }

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;
    void Tick(float deltaTime) override;

    static constexpr float kRowH = 64.0f;

private:
    ProjectTemplateInfo m_Info;
    bool m_Selected = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    SelectFn m_OnSelect;
    ActivateFn m_OnActivate;
    std::chrono::steady_clock::time_point m_LastClick{};
};

class WizardSeparator : public we::runtime::kindui::Widget {
public:
    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
};

[[nodiscard]] bool TemplateMatchesWizardFilter(const ProjectTemplateInfo& tmpl, const std::string& filter);
[[nodiscard]] std::string EstimateTemplateDiskUsage(const ProjectTemplateInfo& tmpl);
[[nodiscard]] std::string TemplateHardwareHint(const ProjectTemplateInfo& tmpl);

} // namespace we::programs::welauncher
