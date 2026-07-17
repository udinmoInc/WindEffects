#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Widget.h"
#include "WindEffects/Runtime/UI/Theming/ThemeToken.h"

#include <functional>
#include <string>

namespace WindEffects::Editor::UI {

// Design-system control variants. All styling comes from ThemeManager StyleRoles.

class UI_API DesignButton : public Widget {
public:
    DesignButton(std::string label, StyleRole role, const char* icon = nullptr);

    void SetOnClicked(std::function<void()> cb) { m_OnClicked = std::move(cb); }
    void SetIcon(const char* icon) { m_Icon = icon; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const Point& position) const override { return IsEnabled() && m_Geometry.Contains(position); }

protected:
    std::string m_Label;
    StyleRole m_Role = StyleRole::ButtonSecondary;
    const char* m_Icon = nullptr;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

class UI_API PrimaryButton : public DesignButton {
public:
    explicit PrimaryButton(std::string label, const char* icon = nullptr)
        : DesignButton(std::move(label), StyleRole::ButtonPrimary, icon) {}
};

class UI_API SecondaryButton : public DesignButton {
public:
    explicit SecondaryButton(std::string label, const char* icon = nullptr)
        : DesignButton(std::move(label), StyleRole::ButtonSecondary, icon) {}
};

class UI_API GhostButton : public DesignButton {
public:
    explicit GhostButton(std::string label, const char* icon = nullptr)
        : DesignButton(std::move(label), StyleRole::ButtonGhost, icon) {}
};

class UI_API ToolbarButton : public DesignButton {
public:
    explicit ToolbarButton(std::string label, const char* icon = nullptr)
        : DesignButton(std::move(label), StyleRole::ToolbarButton, icon) {}
};

class UI_API DangerButton : public DesignButton {
public:
    explicit DangerButton(std::string label, const char* icon = nullptr)
        : DesignButton(std::move(label), StyleRole::ButtonDanger, icon) {}
};

class UI_API IconButton : public Widget {
public:
    explicit IconButton(const char* icon);

    void SetOnClicked(std::function<void()> cb) { m_OnClicked = std::move(cb); }
    void SetActive(bool active) { m_Active = active; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

private:
    const char* m_Icon = nullptr;
    bool m_Active = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

class UI_API Card : public Widget {
public:
    Card() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    float m_HoverAnim = 0.0f;
};

class UI_API SectionHeader : public Widget {
public:
    SectionHeader(std::string title, std::string subtitle = {});

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    std::string m_Title;
    std::string m_Subtitle;
};

class UI_API PropertyRow : public Widget {
public:
    PropertyRow(std::string label, std::string value);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    std::string m_Label;
    std::string m_Value;
};

class UI_API SearchBoxControl : public Widget {
public:
    explicit SearchBoxControl(std::string placeholder = "Search...");

    void SetText(std::string text);
    [[nodiscard]] const std::string& GetText() const { return m_Text; }
    void SetOnChanged(std::function<void(const std::string&)> cb) { m_OnChanged = std::move(cb); }

    [[nodiscard]] bool IsFocusable() const override { return true; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;
    void OnKeyDown(const KeyEvent& event) override;
    void OnFocus() override;
    void OnBlur() override;

private:
    std::string m_Placeholder;
    std::string m_Text;
    float m_HoverAnim = 0.0f;
    std::function<void(const std::string&)> m_OnChanged;
};

class UI_API SidebarItem : public Widget {
public:
    SidebarItem(std::string label, const char* icon = nullptr);

    void SetActive(bool active) { m_Active = active; }
    void SetOnClicked(std::function<void()> cb) { m_OnClicked = std::move(cb); }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

private:
    std::string m_Label;
    const char* m_Icon = nullptr;
    bool m_Active = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

class UI_API WindowHeader : public Widget {
public:
    explicit WindowHeader(std::string title);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    std::string m_Title;
};

class UI_API TableRowBase : public Widget {
public:
    void SetSelected(bool selected) { m_Selected = selected; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

protected:
    bool m_Selected = false;
    float m_HoverAnim = 0.0f;
};

} // namespace WindEffects::Editor::UI
