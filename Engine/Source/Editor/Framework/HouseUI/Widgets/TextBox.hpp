#pragma once

#include "../Core/Widget.hpp"
#include "../Core/Style.hpp"
#include <functional>

namespace HouseEngine::UI {

class TextBox : public Widget {
public:
    TextBox(const std::string& initialText = "", std::function<void(const std::string&)> onTextChanged = nullptr);
    virtual ~TextBox() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;

    void SetText(const std::string& text) { m_Text = text; }
    const std::string& GetText() const { return m_Text; }
    void SetStyle(const WidgetStyle& style) { m_Style = style; }

private:
    std::string m_Text;
    std::function<void(const std::string&)> m_OnTextChanged;

    float m_HoverAnim = 0.0f;
    float m_FocusAnim = 0.0f;

    WidgetStyle m_Style;
};

} // namespace HouseEngine::UI
