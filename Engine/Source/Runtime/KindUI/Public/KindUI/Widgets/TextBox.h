#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Tokens/DesignToken.h"
#include "Text/Editing/TextEditing.h"

#include <functional>
#include <memory>
#include <string>

namespace we::runtime::kindui {

class KINDUI_API TextBox : public Widget {
public:
    TextBox(const std::string& initialText = "", std::function<void(const std::string&)> onTextChanged = nullptr);
    ~TextBox() override = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    [[nodiscard]] bool IsFocusable() const override { return true; }

    void OnKeyDown(const KeyEvent& event) override;
    void OnTextInput(const std::string& utf8);

    void SetText(const std::string& text) {
        if (m_Session) {
            m_Session->SetText(text);
        }
    }
    [[nodiscard]] const std::string& GetText() const {
        static const std::string kEmpty;
        return m_Session ? m_Session->Text() : kEmpty;
    }

private:
    std::unique_ptr<we::runtime::text::editing::ITextEditSession> m_Session;
    std::function<void(const std::string&)> m_OnTextChanged;
    float m_HoverAnim = 0.0f;
    float m_FocusAnim = 0.0f;
    int m_ClickCount = 0;
    bool m_Dragging = false;
};

} // namespace we::runtime::kindui
