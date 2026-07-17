#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Widget.h"
#include <functional>

namespace we::runtime::kindui {

class KINDUI_API TextBox : public Widget {
public:
    TextBox(const std::string& initialText = "", std::function<void(const std::string&)> onTextChanged = nullptr);
    virtual ~TextBox() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    [[nodiscard]] bool IsFocusable() const override { return true; }

    void OnKeyDown(const KeyEvent& event) override;

    void SetText(const std::string& text) { m_Text = text; }
    const std::string& GetText() const { return m_Text; }

private:
    std::string m_Text;
    std::function<void(const std::string&)> m_OnTextChanged;

    float m_HoverAnim = 0.0f;
    float m_FocusAnim = 0.0f;
};

} // namespace we::runtime::kindui
