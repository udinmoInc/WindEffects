#pragma once

#include "KindUI/Core/Widget.h"
#include "Model/WeProjectDescriptor.h"
#include "RHI/Types.h"

#include "Platform/Types.h"

#include <functional>
#include <string>
#include <vector>

namespace we::programs::welauncher {

enum class LauncherPage : int {
    Projects = 0,
    Learn,
    Engine,
    Settings,
    Count
};

enum class ProjectSortMode : int {
    Recent = 0,
    Name,
    Engine
};

class FixedGap : public we::runtime::kindui::Widget {
public:
    explicit FixedGap(float width, float height = 1.0f);
    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

private:
    float m_Width = 0.0f;
    float m_Height = 1.0f;
};

// Global chrome: logo + Help / Settings + window controls (no search).
class LauncherTitleBar : public we::runtime::kindui::Widget {
public:
    using ActionFn = std::function<void()>;

    LauncherTitleBar(we::platform::WindowId window, std::string title);

    void SetLogoTexture(we::rhi::RHIDescriptorSetHandle logoSet);
    void SetOnHelp(ActionFn cb) { m_OnHelp = std::move(cb); }
    void SetOnSettings(ActionFn cb) { m_OnSettings = std::move(cb); }
    void UpdateMaximizeIcon();

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;
    void Tick(float deltaTime) override;

    [[nodiscard]] we::platform::WindowHitTestResult WindowHitTest(we::platform::Int2 point) const;

private:
    enum class HitZone {
        None,
        Drag,
        Help,
        Settings,
        Minimize,
        Maximize,
        Close
    };

    HitZone HitTest(const we::runtime::kindui::Point& p) const;
    void LayoutRects();
    void PaintIconButton(
        we::runtime::kindui::PaintContext& context,
        const we::runtime::kindui::Rect& r,
        const char* icon,
        float hover,
        bool danger = false) const;

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    std::string m_Title;
    we::rhi::RHIDescriptorSetHandle m_LogoSet = we::rhi::RHIDescriptorSetHandle::Invalid;

    ActionFn m_OnHelp;
    ActionFn m_OnSettings;

    HitZone m_HoverZone = HitZone::None;
    HitZone m_PressedZone = HitZone::None;
    float m_HoverMin = 0.0f;
    float m_HoverMax = 0.0f;
    float m_HoverClose = 0.0f;
    float m_HoverHelp = 0.0f;
    float m_HoverSettings = 0.0f;

    we::runtime::kindui::Rect m_BrandRect{};
    we::runtime::kindui::Rect m_HelpRect{};
    we::runtime::kindui::Rect m_SettingsRect{};
    we::runtime::kindui::Rect m_MinRect{};
    we::runtime::kindui::Rect m_MaxRect{};
    we::runtime::kindui::Rect m_CloseRect{};

    bool m_IsMaximized = false;
};

// 1px horizontal rule for Hub-style page separators.
class ThinDivider : public we::runtime::kindui::Widget {
public:
    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
};

// 1px vertical rule (sidebar | content).
class ThinVerticalDivider : public we::runtime::kindui::Widget {
public:
    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
};

class NavSidebar : public we::runtime::kindui::Widget {
public:
    using PageChanged = std::function<void(LauncherPage)>;

    NavSidebar();

    void SetActivePage(LauncherPage page);
    void SetOnPageChanged(PageChanged cb) { m_OnPageChanged = std::move(cb); }
    void SetCollapsed(bool collapsed);
    [[nodiscard]] bool IsCollapsed() const { return m_Collapsed; }
    [[nodiscard]] float PreferredWidth() const;
    [[nodiscard]] LauncherPage GetActivePage() const { return m_Active; }
    bool NavigateByDelta(int delta);
    bool NavigateToIndex(int index);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    struct Item {
        LauncherPage page;
        const char* label;
        const char* icon;
        float hoverAnim = 0.0f;
        float selectAnim = 0.0f;
    };

    int HitItem(const we::runtime::kindui::Point& p) const;
    we::runtime::kindui::Rect ItemRect(int index) const;
    int IndexOfPage(LauncherPage page) const;

    std::vector<Item> m_Items;
    LauncherPage m_Active = LauncherPage::Projects;
    PageChanged m_OnPageChanged;
    bool m_Collapsed = false;
    int m_PressedIndex = -1;
    int m_HoveredIndex = -1;
    float m_WidthAnim = 200.0f;
};

class SearchField : public we::runtime::kindui::Widget {
public:
    using TextChanged = std::function<void(const std::string&)>;

    SearchField();

    void SetText(const std::string& text);
    [[nodiscard]] const std::string& GetText() const { return m_Text; }
    void SetPlaceholder(std::string placeholder) { m_Placeholder = std::move(placeholder); }
    void SetOnTextChanged(TextChanged cb) { m_OnTextChanged = std::move(cb); }
    void AppendCodepoint(char32_t codepoint);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnKeyDown(const we::runtime::kindui::KeyEvent& event) override;
    void OnFocus() override;
    void OnBlur() override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    we::runtime::kindui::Rect ClearRect() const;

    std::string m_Text;
    std::string m_Placeholder = "Search...";
    TextChanged m_OnTextChanged;
    float m_HoverAnim = 0.0f;
    float m_FocusAnim = 0.0f;
    float m_CaretBlink = 0.0f;
    bool m_ShowCaret = true;
};

class SegmentedControl : public we::runtime::kindui::Widget {
public:
    using Changed = std::function<void(int)>;

    SegmentedControl(std::vector<std::string> labels, std::vector<std::string> icons = {});

    void SetSelected(int index);
    [[nodiscard]] int GetSelected() const { return m_Selected; }
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    int HitSegment(const we::runtime::kindui::Point& p) const;
    we::runtime::kindui::Rect SegmentRect(int index) const;

    std::vector<std::string> m_Labels;
    std::vector<std::string> m_Icons;
    int m_Selected = 0;
    int m_Hovered = -1;
    Changed m_OnChanged;
    std::vector<float> m_HoverAnims;
};

class StatusFooter : public we::runtime::kindui::Widget {
public:
    void SetStatus(std::string status);
    void SetEngineLabel(std::string label);
    void SetSdkSummary(std::string summary);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

private:
    std::string m_Status = "Ready";
    std::string m_EngineLabel;
    std::string m_SdkSummary;
};

class SectionCard : public we::runtime::kindui::Widget {
public:
    SectionCard();

    void SetTitle(std::string title) { m_Title = std::move(title); }
    void SetContent(const std::shared_ptr<we::runtime::kindui::Widget>& content);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    std::string m_Title;
    std::shared_ptr<we::runtime::kindui::Widget> m_Content;
    float m_HeaderHeight = 32.0f;
};

class EmptyStatePanel : public we::runtime::kindui::Widget {
public:
    EmptyStatePanel(std::string title, std::string subtitle, const char* iconName = nullptr);

    void SetPrimaryAction(std::string label, const char* icon, std::function<void()> onClick);
    void SetSecondaryAction(std::string label, const char* icon, std::function<void()> onClick);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    enum class HitZone { None, Primary, Secondary };
    HitZone HitTest(const we::runtime::kindui::Point& p) const;

    std::string m_Title;
    std::string m_Subtitle;
    const char* m_Icon = nullptr;
    std::string m_PrimaryLabel;
    std::string m_SecondaryLabel;
    const char* m_PrimaryIcon = nullptr;
    const char* m_SecondaryIcon = nullptr;
    std::function<void()> m_OnPrimary;
    std::function<void()> m_OnSecondary;
    we::runtime::kindui::Rect m_PrimaryRect{};
    we::runtime::kindui::Rect m_SecondaryRect{};
    HitZone m_Hover = HitZone::None;
    HitZone m_Pressed = HitZone::None;
};

// Zero-project welcome surface: centered CTA + default projects folder row.
class ProjectsEmptyState : public we::runtime::kindui::Widget {
public:
    using ActionFn = std::function<void()>;

    void SetFolderPath(std::string path);
    void SetOnNewProject(ActionFn cb) { m_OnNew = std::move(cb); }
    void SetOnOpenProject(ActionFn cb) { m_OnOpen = std::move(cb); }
    void SetOnChangeFolder(ActionFn cb) { m_OnChangeFolder = std::move(cb); }

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;

private:
    enum class HitZone { None, New, Open, Change };
    HitZone HitTest(const we::runtime::kindui::Point& p) const;
    void LayoutContent();

    std::string m_FolderPath;
    ActionFn m_OnNew;
    ActionFn m_OnOpen;
    ActionFn m_OnChangeFolder;
    we::runtime::kindui::Rect m_NewRect{};
    we::runtime::kindui::Rect m_OpenRect{};
    we::runtime::kindui::Rect m_ChangeRect{};
    HitZone m_Hover = HitZone::None;
    HitZone m_Pressed = HitZone::None;
};

// Compact toolbar search field (Projects page).
class CompactSearchField : public we::runtime::kindui::Widget {
public:
    using ChangedFn = std::function<void(const std::string&)>;

    explicit CompactSearchField(std::string placeholder = "Search Projects...");

    void SetText(std::string text);
    [[nodiscard]] const std::string& GetText() const { return m_Text; }
    void SetOnChanged(ChangedFn cb) { m_OnChanged = std::move(cb); }
    void AppendCodepoint(char32_t codepoint);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnKeyDown(const we::runtime::kindui::KeyEvent& event) override;
    void OnFocus() override;
    void OnBlur() override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;

private:
    std::string m_Placeholder;
    std::string m_Text;
    float m_HoverAnim = 0.0f;
    ChangedFn m_OnChanged;
};

} // namespace we::programs::welauncher
