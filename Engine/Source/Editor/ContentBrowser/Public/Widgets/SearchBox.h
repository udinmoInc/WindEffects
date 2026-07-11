#pragma once

#include "ContentBrowser/Export.h"

#include "Core/Widget.h"
#include "Core/Style.h"
#include "Core/Icon.h"
#include <string>
#include <functional>

namespace WindEffects::Editor::UI {

// Search box widget with icon and clear button
class CONTENTBROWSER_API SearchBox : public Widget {
public:
    using OnTextChanged = std::function<void(const std::string&)>;

    SearchBox();
    virtual ~SearchBox() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;
    void OnFocus() override;
    void OnBlur() override;

    // Text management
    void SetText(const std::string& text);
    std::string GetText() const { return m_Text; }
    void SetPlaceholder(const std::string& placeholder) { m_Placeholder = placeholder; }

    // Callbacks
    void SetOnTextChanged(OnTextChanged callback) { m_OnTextChanged = callback; }

    // Styling
    void SetFillWidth(bool fill) { m_FillWidth = fill; }
    void SetWidth(float width) { m_Width = width; }

private:
    void UpdateCaretBlink(float deltaTime);
    Rect GetTextRect() const;
    Rect GetClearButtonRect() const;

    std::string m_Text;
    std::string m_Placeholder = "Search...";
    size_t m_CaretPosition = 0;
    
    float m_Height = 35.0f;
    float m_Width = 240.0f;
    bool m_FillWidth = false;
    float m_CaretBlinkTime = 0.0f;
    bool m_ShowCaret = true;
    
    OnTextChanged m_OnTextChanged;

    WidgetStyle m_Style;
};

} // namespace we::editor::contentbrowser::UI
