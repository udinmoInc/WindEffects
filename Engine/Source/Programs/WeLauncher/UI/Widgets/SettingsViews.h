#pragma once

#include "WindEffects/Runtime/UI/Core/Widget.h"
#include "WindEffects/Runtime/UI/Theming/ThemeToken.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace we::programs::welauncher {

enum class SettingsCategory : int {
    General = 0,
    Engine,
    Storage,
    FileAssociations,
    About,
    Count
};

[[nodiscard]] const char* SettingsCategoryTitle(SettingsCategory category);
[[nodiscard]] const char* SettingsCategoryKeywords(SettingsCategory category);

// Compact titled section with header + description + subtle separator (not a card).
class SettingsGroup : public WindEffects::Editor::UI::Widget {
public:
    SettingsGroup(std::string title, std::string description = {});

    void SetContent(const std::shared_ptr<WindEffects::Editor::UI::Widget>& content);
    void SetHighlighted(bool highlighted) { m_Highlighted = highlighted; }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    std::string m_Title;
    std::string m_Description;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_Content;
    bool m_Highlighted = false;
};

// Label + trailing control row (content-sized, never vertically stretched).
class SettingsRow : public WindEffects::Editor::UI::Widget {
public:
    SettingsRow(
        std::string label,
        std::string hint,
        std::shared_ptr<WindEffects::Editor::UI::Widget> control,
        std::string searchText = {});

    // Must be called after the row is owned by shared_ptr (not from the constructor).
    void AttachControl();
    [[nodiscard]] const std::string& SearchText() const { return m_SearchText; }
    void SetHighlighted(bool highlighted) { m_Highlighted = highlighted; }
    [[nodiscard]] bool MatchesQuery(const std::string& queryLower) const;

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

    static constexpr float kRowH = 40.0f;

private:
    std::string m_Label;
    std::string m_Hint;
    std::string m_SearchText;
    std::shared_ptr<WindEffects::Editor::UI::Widget> m_Control;
    bool m_Highlighted = false;
};

class ToggleSwitch : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(bool)>;

    explicit ToggleSwitch(bool on = false);

    void SetOn(bool on);
    [[nodiscard]] bool IsOn() const { return m_On; }
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    bool m_On = false;
    bool m_Pressed = false;
    float m_Anim = 0.0f;
    Changed m_OnChanged;
};

class SettingsCheckBox : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(bool)>;

    explicit SettingsCheckBox(bool checked = false);

    void SetChecked(bool checked);
    [[nodiscard]] bool IsChecked() const { return m_Checked; }
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    bool m_Checked = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    Changed m_OnChanged;
};

class SettingsDropdown : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(int)>;

    SettingsDropdown(std::vector<std::string> options, int selected = 0);

    void SetSelected(int index);
    [[nodiscard]] int GetSelected() const { return m_Selected; }
    [[nodiscard]] const std::string& SelectedLabel() const;
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    [[nodiscard]] int HitOption(const WindEffects::Editor::UI::Point& p) const;
    [[nodiscard]] WindEffects::Editor::UI::Rect MenuRect() const;
    [[nodiscard]] WindEffects::Editor::UI::Rect OptionRect(int index) const;

    std::vector<std::string> m_Options;
    int m_Selected = 0;
    bool m_Open = false;
    int m_HoverOption = -1;
    float m_HoverAnim = 0.0f;
    Changed m_OnChanged;
};

class SettingsSegmented : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(int)>;

    explicit SettingsSegmented(std::vector<std::string> labels);

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
    [[nodiscard]] int HitSegment(const WindEffects::Editor::UI::Point& p) const;
    [[nodiscard]] WindEffects::Editor::UI::Rect SegmentRect(int index) const;

    std::vector<std::string> m_Labels;
    int m_Selected = 0;
    int m_Hovered = -1;
    Changed m_OnChanged;
    std::vector<float> m_HoverAnims;
};

class SettingsTextField : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(const std::string&)>;

    explicit SettingsTextField(std::string text = {}, float preferredWidth = 220.0f);

    void SetText(std::string text);
    [[nodiscard]] const std::string& GetText() const { return m_Text; }
    void SetPlaceholder(std::string placeholder) { m_Placeholder = std::move(placeholder); }
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }

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
    std::string m_Text;
    std::string m_Placeholder;
    float m_PreferredWidth = 220.0f;
    float m_HoverAnim = 0.0f;
    float m_FocusAnim = 0.0f;
    float m_CaretBlink = 0.0f;
    Changed m_OnChanged;
};

class PathPickerField : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(const std::string&)>;

    PathPickerField(std::string path, bool folderMode = true);

    void SetPath(std::string path);
    [[nodiscard]] const std::string& GetPath() const { return m_Path; }
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }
    void SetDialogTitle(std::string title) { m_DialogTitle = std::move(title); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    [[nodiscard]] WindEffects::Editor::UI::Rect BrowseRect() const;
    void OpenPicker();

    std::string m_Path;
    std::string m_DialogTitle = "Select Folder";
    bool m_FolderMode = true;
    Changed m_OnChanged;
    bool m_HoverBrowse = false;
    bool m_PressedBrowse = false;
    float m_HoverAnim = 0.0f;
};

class ColorSwatchPicker : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(const std::string&)>;

    explicit ColorSwatchPicker(std::string hexColor);

    void SetColorHex(std::string hex);
    [[nodiscard]] const std::string& GetColorHex() const { return m_Hex; }
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    [[nodiscard]] int HitSwatch(const WindEffects::Editor::UI::Point& p) const;
    [[nodiscard]] WindEffects::Editor::UI::Rect SwatchRect(int index) const;

    std::string m_Hex;
    int m_Selected = 0;
    int m_Hovered = -1;
    Changed m_OnChanged;
    std::vector<float> m_HoverAnims;
};

class NumberStepper : public WindEffects::Editor::UI::Widget {
public:
    using Changed = std::function<void(float)>;

    NumberStepper(float value, float minValue, float maxValue, float step, std::string suffix = {});

    void SetValue(float value);
    [[nodiscard]] float GetValue() const { return m_Value; }
    void SetOnChanged(Changed cb) { m_OnChanged = std::move(cb); }

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseUp(const WindEffects::Editor::UI::MouseEvent& event) override;
    bool ShowsPointerCursor(const WindEffects::Editor::UI::Point& position) const override;
    void Tick(float deltaTime) override;

private:
    enum class Zone { None, Minus, Plus };
    [[nodiscard]] Zone HitTest(const WindEffects::Editor::UI::Point& p) const;
    [[nodiscard]] WindEffects::Editor::UI::Rect MinusRect() const;
    [[nodiscard]] WindEffects::Editor::UI::Rect PlusRect() const;

    float m_Value = 0.0f;
    float m_Min = 0.0f;
    float m_Max = 100.0f;
    float m_Step = 1.0f;
    std::string m_Suffix;
    Zone m_Hover = Zone::None;
    Zone m_Pressed = Zone::None;
    Changed m_OnChanged;
};

class CacheUsageBar : public WindEffects::Editor::UI::Widget {
public:
    CacheUsageBar(std::string label, float usedMb, float capacityMb);

    void SetUsage(float usedMb, float capacityMb);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;

private:
    std::string m_Label;
    float m_UsedMb = 0.0f;
    float m_CapacityMb = 1.0f;
};

class AppearancePreviewPanel : public WindEffects::Editor::UI::Widget {
public:
    void SetTheme(std::string theme);
    void SetAccentHex(std::string hex);
    void SetIconStyle(std::string style);
    void SetUiScale(float scale);
    void SetFontSize(float size);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    std::string m_Theme = "Graphite Dark";
    std::string m_AccentHex = "#5B8DEF";
    std::string m_IconStyle = "Outline";
    float m_UiScale = 1.0f;
    float m_FontSize = 13.0f;
    float m_Pulse = 0.0f;
};

class SettingsActionBar : public WindEffects::Editor::UI::Widget {
public:
    void AddAction(
        std::string label,
        const char* icon,
        std::function<void()> onClick,
        bool primary = false);

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    std::vector<std::shared_ptr<WindEffects::Editor::UI::Widget>> m_Buttons;
};

[[nodiscard]] WindEffects::Editor::UI::Color ParseHexColor(const std::string& hex);
[[nodiscard]] int IndexOfOption(const std::vector<std::string>& options, const std::string& value);

} // namespace we::programs::welauncher
