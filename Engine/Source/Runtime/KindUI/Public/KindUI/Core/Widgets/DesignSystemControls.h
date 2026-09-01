#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <functional>
#include <string>

namespace we::runtime::kindui {

// Design-system control variants. All styling comes from ThemeManager StyleRoles.

class KINDUI_API DesignButton : public Widget {
public:
    DesignButton(std::string label, StyleRole role, WindIconRef icon = kWindIconNone);

    void SetOnClicked(std::function<void()> cb) { m_OnClicked = std::move(cb); }
    void SetLabel(std::string label);
    void SetIcon(WindIconRef icon) { m_Icon = icon; }

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
    WindIconRef m_Icon = kWindIconNone;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

class KINDUI_API PrimaryButton : public DesignButton {
public:
    explicit PrimaryButton(std::string label, WindIconRef icon = kWindIconNone)
        : DesignButton(std::move(label), StyleRole::ButtonPrimary, icon) {}
};

class KINDUI_API SecondaryButton : public DesignButton {
public:
    explicit SecondaryButton(std::string label, WindIconRef icon = kWindIconNone)
        : DesignButton(std::move(label), StyleRole::ButtonSecondary, icon) {}
};

class KINDUI_API GhostButton : public DesignButton {
public:
    explicit GhostButton(std::string label, WindIconRef icon = kWindIconNone)
        : DesignButton(std::move(label), StyleRole::ButtonGhost, icon) {}
};

class KINDUI_API ToolbarButton : public DesignButton {
public:
    explicit ToolbarButton(std::string label, WindIconRef icon = kWindIconNone)
        : DesignButton(std::move(label), StyleRole::ToolbarButton, icon) {}
};

class KINDUI_API DangerButton : public DesignButton {
public:
    explicit DangerButton(std::string label, WindIconRef icon = kWindIconNone)
        : DesignButton(std::move(label), StyleRole::ButtonDanger, icon) {}
};

class KINDUI_API IconButton : public Widget {
public:
    explicit IconButton(WindIconRef icon = kWindIconNone);

    void SetOnClicked(std::function<void()> cb) { m_OnClicked = std::move(cb); }
    void SetActive(bool active) { m_Active = active; }
    void SetBorderless(bool borderless) { m_Borderless = borderless; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

private:
    WindIconRef m_Icon = kWindIconNone;
    bool m_Active = false;
    bool m_Borderless = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

class KINDUI_API Card : public Widget {
public:
    Card() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    float m_HoverAnim = 0.0f;
};

class KINDUI_API SectionHeader : public Widget {
public:
    SectionHeader(std::string title, std::string subtitle = {});

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    std::string m_Title;
    std::string m_Subtitle;
};

class KINDUI_API PropertyRow : public Widget {
public:
    PropertyRow(std::string label, std::string value);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    std::string m_Label;
    std::string m_Value;
};

class KINDUI_API SearchBoxControl : public Widget {
public:
    explicit SearchBoxControl(std::string placeholder = "Search...");

    void SetText(std::string text);
    [[nodiscard]] const std::string& GetText() const { return m_Text; }
    void SetOnChanged(std::function<void(const std::string&)> cb) { m_OnChanged = std::move(cb); }
    void SetToolbarFlat(bool flat) { m_ToolbarFlat = flat; }
    void SetToolbarInset(bool inset);

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
    bool m_ToolbarFlat = false;
    bool m_ToolbarInset = false;
    std::function<void(const std::string&)> m_OnChanged;
};

class KINDUI_API PanelTab : public Widget {
public:
    explicit PanelTab(std::string label);

    void SetActive(bool active) { m_Active = active; InvalidatePaint(); }
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
    bool m_Active = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

class KINDUI_API SidebarItem : public Widget {
public:
    SidebarItem(std::string label, WindIconRef icon = kWindIconNone);

    void SetActive(bool active) { m_Active = active; }
    void SetOnClicked(std::function<void()> cb) { m_OnClicked = std::move(cb); }
    void SetLabel(std::string label) { m_Label = std::move(label); InvalidatePaint(); }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

private:
    std::string m_Label;
    WindIconRef m_Icon = kWindIconNone;
    bool m_Active = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    std::function<void()> m_OnClicked;
};

class KINDUI_API WindowHeader : public Widget {
public:
    explicit WindowHeader(std::string title);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    std::string m_Title;
};

class KINDUI_API TableRowBase : public Widget {
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

[[nodiscard]] KINDUI_API std::shared_ptr<PrimaryButton> MakePrimaryAction(
    std::string label,
    WindIconRef icon = kWindIconNone);
[[nodiscard]] KINDUI_API std::shared_ptr<SecondaryButton> MakeSecondaryAction(
    std::string label,
    WindIconRef icon = kWindIconNone);
[[nodiscard]] KINDUI_API std::shared_ptr<PanelTab> MakePanelTab(std::string label);

} // namespace we::runtime::kindui
