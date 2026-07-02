#pragma once

#include "Core/Widget.hpp"
#include <functional>
#include <string>
#include <vector>

namespace we::programs::editor {

class FirstRunAgreementPopup : public we::UI::Widget {
public:
    explicit FirstRunAgreementPopup(std::string markdownAndTextContent);

    void SetOnAccepted(std::function<void()> callback) { m_OnAccepted = std::move(callback); }
    void SetOnDeclined(std::function<void()> callback) { m_OnDeclined = std::move(callback); }
    void SetViewportSize(float width, float height);

    we::UI::Size Measure(const we::UI::Size& availableSize) override;
    void Arrange(const we::UI::Rect& allottedRect) override;
    void Paint(we::UI::PaintContext& context) override;

    void OnMouseDown(const we::UI::MouseEvent& event) override;
    void OnMouseMove(const we::UI::MouseEvent& event) override;
    void OnMouseUp(const we::UI::MouseEvent& event) override;
    void OnMouseWheel(const we::UI::MouseEvent& event) override;
    void OnKeyDown(const we::UI::KeyEvent& event) override;
    bool ShowsPointerCursor(const we::UI::Point& position) const override;

private:
    struct DisplayLine {
        std::string text;
        float fontSize = 12.0f;
        bool bold = false;
        we::UI::Color color = we::UI::Color::White();
    };

    struct ButtonState {
        we::UI::Rect rect{};
        std::string label;
        std::string iconName;
        bool hovered = false;
        bool pressed = false;
    };

    void RebuildDisplayLines(we::UI::PaintContext& context, float maxWidth);
    std::vector<std::string> WrapLine(we::UI::PaintContext& context, const std::string& text, float fontSize, float maxWidth) const;
    static std::string NormalizeMarkdownLine(const std::string& line);
    static std::string ExtractTitleFromMarkdown(const std::string& content);

public:
    static std::string BuildDefaultAgreementContent();

    std::string m_Title;
    std::string m_RawContent;
    std::vector<DisplayLine> m_DisplayLines;
    float m_CachedWrapWidth = -1.0f;

    float m_ViewportWidth = 1280.0f;
    float m_ViewportHeight = 720.0f;
    float m_ScrollOffset = 0.0f;
    float m_TotalContentHeight = 0.0f;

    we::UI::Rect m_DialogRect{};
    we::UI::Rect m_ContentRect{};
    ButtonState m_CopyButton{};
    ButtonState m_AgreeButton{};
    ButtonState m_DeclineButton{};

    std::function<void()> m_OnAccepted;
    std::function<void()> m_OnDeclined;
};

bool HasAcceptedFirstRunAgreement();
void SetAcceptedFirstRunAgreement(bool accepted);
std::string LoadFirstRunAgreementContent();
std::string LoadFirstRunAgreementEulaContent();
std::string LoadFirstRunAgreementThirdPartyNoticesContent();

} // namespace we::programs::editor

