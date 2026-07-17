#pragma once

#include "Core/Widget.h"
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

enum class ProjectViewMode : int {
    Grid = 0,
    List
};

enum class ProjectSortMode : int {
    Recent = 0,
    Name,
    Engine
};

class FixedGap : public WindEffects::Editor::UI::Widget {
public:
    explicit FixedGap(float width, float height = 1.0f);
    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;

private:
    float m_Width = 0.0f;
    float m_Height = 1.0f;
};

// Global chrome: logo + Help / Settings + window controls (no search).
class LauncherTitleBar : public WindEffects::Editor::UI::Widget {
public:
    using ActionFn = std::function<void()>;

    LauncherTitleBar(we::platform::WindowId window, std::string title);

    void SetLogoTexture(we::rhi::RHIDescriptorSetHandle logoSet);
    void SetOnHelp(ActionFn cb) { m_OnHelp = std::move(cb); }
    void SetOnSettings(ActionFn cb) { m_OnSettings = std::move(cb); }
    void UpdateMaximizeIcon();

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
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

    HitZone HitTest(const WindEffects::Editor::UI::Point& p) const;
    void LayoutRects();
    void PaintIconButton(
        WindEffects::Editor::UI::PaintContext& context,
        const WindEffects::Editor::UI::Rect& r,
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

    WindEffects::Editor::UI::Rect m_BrandRect{};
    WindEffects::Editor::UI::Rect m_HelpRect{};
    WindEffects::Editor::UI::Rect m_SettingsRect{};
    WindEffects::Editor::UI::Rect m_MinRect{};
    WindEffects::Editor::UI::Rect m_MaxRect{};
    WindEffects::Editor::UI::Rect m_CloseRect{};

    bool m_IsMaximized = false;
};

// 1px horizontal rule for Hub-style page separators.
class ThinDivider : public WindEffects::Editor::UI::Widget {
public:
    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
};

// 1px vertical rule (sidebar | content).
class ThinVerticalDivider : public WindEffects::Editor::UI::Widget {
public:
    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
};

class NavSidebar : public WindEffects::Editor::UI::Widget {
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

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    struct Item {
        LauncherPage page;
        const char* label;
        const char* icon;
        float hoverAnim = 0.0f;
        float selectAnim = 0.0f;
    };

    int HitItem(const WindEffects::Editor::UI::Point& p) const;
    WindEffects::Editor::UI::Rect ItemRect(int index) const;
    int IndexOfPage(LauncherPage page) const;

    std::vector<Item> m_Items;
    LauncherPage m_Active = LauncherPage::Projects;
    PageChanged m_OnPageChanged;
    bool m_Collapsed = false;
    int m_PressedIndex = -1;
    int m_HoveredIndex = -1;
    float m_WidthAnim = 200.0f;
};

class SearchField : public WindEffects::Editor::UI::Widget {
public:
    using TextChanged = std::function<void(const std::string&)>;

    SearchField();

    void SetText(const std::string& text);
    [[nodiscard]] const std::string& GetText() const { return m_Text; }
    void SetPlaceholder(std::string placeholder) { m_Placeholder = std::move(placeholder); }
    void SetOnTextChanged(TextChanged cb) { m_OnTextChanged = std::move(cb); }
    void AppendCodepoint(char32_t codepoint);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnKeyDown(const WindEffects::Editor::UI::KeyEvent& event) override;
    void OnFocus() override;
    void OnBlur() override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    WindEffects::Editor::UI::Rect ClearRect() const;

    std::string m_Text;
    std::string m_Placeholder = "Search...";
    TextChanged m_OnTextChanged;
    float m_HoverAnim = 0.0f;
    float m_FocusAnim = 0.0f;
    float m_CaretBlink = 0.0f;
    bool m_ShowCaret = true;
};

class SegmentedControl : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(int)>;

    SegmentedControl(std::vector<std::string> labels, std::vector<std::string> icons = {});

    void SetSelected(int index);
    [[nodiscard]] int GetSelected() const { return m_Selected; }
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    int HitSegment(const WindEffects::Editor::UI::Point& p) const;
    WindEffects::Editor::UI::Rect SegmentRect(int index) const;

    std::vector<std::string> m_Labels;
    std::vector<std::string> m_Icons;
    int m_Selected = 0;
    int m_Hovered = -1;
    Changed m_OnChanged;
    std::vector<float> m_HoverAnims;
};

class StatusFooter : public WindEffects::Editor::UI::Widget {
public:
    void SetStatus(std::string status);
    void SetEngineLabel(std::string label);
    void SetSdkSummary(std::string summary);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;

private:
    std::string m_Status = "Ready";
    std::string m_EngineLabel;
    std::string m_SdkSummary;
};

class SectionCard : public WindEffects::Editor::UI::Widget {
public:
    SectionCard();

    void SetTitle(std::string title) { m_Title = std::move(title); }
    void SetContent(const std::shared_ptr<WindEffects::Editor::UI::Widget>& content);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    std::string m_Title;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_Content;
    float m_HeaderHeight = 32.0f;
};

class EmptyStatePanel : public WindEffects::Editor::UI::Widget {
public:
    EmptyStatePanel(std::string title, std::string subtitle, const char* iconName = nullptr);

    void SetPrimaryAction(std::string label, const char* icon, std::function<void()> onClick);
    void SetSecondaryAction(std::string label, const char* icon, std::function<void()> onClick);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    enum class HitZone { None, Primary, Secondary };
    HitZone HitTest(const WindEffects::Editor::UI::Point& p) const;

    std::string m_Title;
    std::string m_Subtitle;
    const char* m_Icon = nullptr;
    std::string m_PrimaryLabel;
    std::string m_SecondaryLabel;
    const char* m_PrimaryIcon = nullptr;
    const char* m_SecondaryIcon = nullptr;
    std::function<void()> m_OnPrimary;
    std::function<void()> m_OnSecondary;
    WindEffects::Editor::UI::Rect m_PrimaryRect{};
    WindEffects::Editor::UI::Rect m_SecondaryRect{};
    HitZone m_Hover = HitZone::None;
    HitZone m_Pressed = HitZone::None;
};

// Zero-project welcome surface: centered CTA + default projects folder row.
class ProjectsEmptyState : public WindEffects::Editor::UI::Widget {
public:
    using ActionFn = std::function<void()>;

    void SetFolderPath(std::string path);
    void SetOnNewProject(ActionFn cb) { m_OnNew = std::move(cb); }
    void SetOnOpenProject(ActionFn cb) { m_OnOpen = std::move(cb); }
    void SetOnChangeFolder(ActionFn cb) { m_OnChangeFolder = std::move(cb); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;

private:
    enum class HitZone { None, New, Open, Change };
    HitZone HitTest(const WindEffects::Editor::UI::Point& p) const;
    void LayoutContent();

    std::string m_FolderPath;
    ActionFn m_OnNew;
    ActionFn m_OnOpen;
    ActionFn m_OnChangeFolder;
    WindEffects::Editor::UI::Rect m_NewRect{};
    WindEffects::Editor::UI::Rect m_OpenRect{};
    WindEffects::Editor::UI::Rect m_ChangeRect{};
    HitZone m_Hover = HitZone::None;
    HitZone m_Pressed = HitZone::None;
};

// Compact toolbar search field (Projects page).
class CompactSearchField : public WindEffects::Editor::UI::Widget {
public:
    using ChangedFn = std::function<void(const std::string&)>;

    explicit CompactSearchField(std::string placeholder = "Search Projects...");

    void SetText(std::string text);
    [[nodiscard]] const std::string& GetText() const { return m_Text; }
    void SetOnChanged(ChangedFn cb) { m_OnChanged = std::move(cb); }
    void AppendCodepoint(char32_t codepoint);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnKeyDown(const WindEffects::Editor::UI::KeyEvent& event) override;
    void OnFocus() override;
    void OnBlur() override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;

private:
    std::string m_Placeholder;
    std::string m_Text;
    float m_HoverAnim = 0.0f;
    ChangedFn m_OnChanged;
};

class ModalOverlay : public WindEffects::Editor::UI::Widget {
public:
    void SetDialog(const std::shared_ptr<WindEffects::Editor::UI::Widget>& dialog);
    void SetDialogWidth(float width) { m_DialogWidth = width; }
    void SetDialogHeight(float height) { m_DialogHeight = height; }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void Tick(float deltaTime) override;

    void SetOnScrimClicked(std::function<void()> cb) { m_OnScrimClicked = std::move(cb); }

private:
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_Dialog;
    float m_DialogWidth = 520.0f;
    float m_DialogHeight = 0.0f; // 0 = size to content
    std::function<void()> m_OnScrimClicked;
};

} // namespace we::programs::welauncher
