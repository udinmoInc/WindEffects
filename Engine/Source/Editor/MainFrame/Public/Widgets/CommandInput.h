#pragma once

#include "Core/Widget.h"
#include <functional>
#include <string>

namespace we::UI {

// Compact command entry field for the status bar footer.
class CommandInput : public Widget {
public:
    using OnCommandSubmitted = std::function<void(const std::string& command)>;

    CommandInput();
    ~CommandInput() override = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;
    void OnFocus() override;
    void OnBlur() override;

    void SetPlaceholder(const std::string& placeholder) { m_Placeholder = placeholder; }
    void SetOnCommandSubmitted(OnCommandSubmitted callback) { m_OnCommandSubmitted = std::move(callback); }

private:
    std::string m_Text;
    std::string m_Placeholder = "Enter command...";
    size_t m_CaretPosition = 0;
    bool m_ShowCaret = false;

    float m_Height = 24.0f;
    float m_Width = 280.0f;

    OnCommandSubmitted m_OnCommandSubmitted;
};

} // namespace we::UI
