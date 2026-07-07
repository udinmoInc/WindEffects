#pragma once

#include "Core/Widget.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace we::programs::editor {

class FirstRunAgreementPopup : public we::UI::Widget {
public:
    explicit FirstRunAgreementPopup(std::string markdownAndTextContent);
    ~FirstRunAgreementPopup();

    void SetOnAccepted(std::function<void()> callback) { m_OnAccepted = std::move(callback); }
    void SetOnDeclined(std::function<void()> callback) { m_OnDeclined = std::move(callback); }

    we::UI::Size Measure(const we::UI::Size& availableSize) override;
    void Arrange(const we::UI::Rect& allottedRect) override;
    void Paint(we::UI::PaintContext& context) override;

    void OnMouseDown(const we::UI::MouseEvent& event) override;
    void OnMouseMove(const we::UI::MouseEvent& event) override;
    void OnMouseUp(const we::UI::MouseEvent& event) override;
    void OnMouseWheel(const we::UI::MouseEvent& event) override;
    void OnKeyDown(const we::UI::KeyEvent& event) override;
    bool ShowsPointerCursor(const we::UI::Point& position) const override;
    
    void ExecutePendingCallback();

private:
    // Document node types
    enum class NodeType {
        Heading1,
        Heading2,
        Heading3,
        Paragraph,
        CodeBlock,
        Blockquote,
        UnorderedList,
        OrderedList,
        HorizontalRule,
        Spacer
    };

    // Inline text style
    enum class TextStyle {
        Normal,
        Bold,
        Italic,
        Code,
        Link
    };

    struct TextRun {
        std::string text;
        TextStyle style = TextStyle::Normal;
        std::string linkUrl;
    };

    struct DocumentNode {
        NodeType type = NodeType::Paragraph;
        std::vector<TextRun> runs;
        std::string rawText; // For code blocks
        int listNumber = 0; // For ordered lists
        float marginTop = 0.0f;
        float marginBottom = 0.0f;
        
        // Cached layout data
        float cachedHeight = 0.0f;
        float cachedWidth = -1.0f;
        std::vector<std::string> wrappedLines;
    };

    struct ButtonState {
        we::UI::Rect rect{};
        std::string label;
        std::string iconName;
        bool hovered = false;
        bool pressed = false;
    };

    struct ScrollbarState {
        we::UI::Rect track{};
        we::UI::Rect thumb{};
        bool hovering = false;
        bool dragging = false;
        float dragStartY = 0.0f;
        float dragStartScroll = 0.0f;
    };

    // Document parsing
    void ParseDocument();
    std::vector<TextRun> ParseInlineText(const std::string& text);
    static std::string StripMarkdown(const std::string& text);
    static std::string ExtractTitle(const std::string& content);

    // Layout engine
    void ComputeLayout(we::UI::PaintContext& context);
    float MeasureNode(we::UI::PaintContext& context, DocumentNode& node, float maxWidth);
    std::vector<std::string> WrapWords(we::UI::PaintContext& context, const std::string& text, float fontSize, float maxWidth);

    // Rendering
    void RenderNode(we::UI::PaintContext& context, const DocumentNode& node, float y);
    void RenderTextRuns(we::UI::PaintContext& context, const std::vector<TextRun>& runs, float x, float& y, float fontSize, const we::UI::Color& baseColor, float maxWidth);
    void RenderCodeBlock(we::UI::PaintContext& context, const DocumentNode& node, float y);
    void RenderBlockquote(we::UI::PaintContext& context, const DocumentNode& node, float y);
    void RenderList(we::UI::PaintContext& context, const DocumentNode& node, float y);

    // Scrolling
    void SetScrollOffset(float offset);
    void ScrollBy(float delta);
    void ScrollPageUp();
    void ScrollPageDown();
    void ScrollToTop();
    void ScrollToBottom();
    void UpdateScrollbarGeometry();
    bool IsOverScrollbar(const we::UI::Point& point) const;
    bool IsOverThumb(const we::UI::Point& point) const;

    // Utilities
    float GetFontSize(NodeType type) const;
    float GetLineHeight(NodeType type) const;
    we::UI::Color GetTextColor(NodeType type) const;

public:
    static std::string BuildDefaultContent();

    // Document
    std::string m_Title;
    std::string m_RawContent;
    std::vector<DocumentNode> m_DocumentNodes;
    std::vector<float> m_NodeYPositions;
    float m_TotalDocumentHeight = 0.0f;
    float m_LayoutWidth = -1.0f;
    bool m_LayoutValid = false;

    // Layout
    we::UI::Rect m_DialogRect{};
    we::UI::Rect m_ContentRect{};
    float m_ContentMargin = 16.0f;

    // Scrolling
    float m_ScrollOffset = 0.0f;
    ScrollbarState m_Scrollbar{};

    // Buttons
    ButtonState m_CopyButton{};
    ButtonState m_AgreeButton{};
    ButtonState m_DeclineButton{};

    // Callbacks
    std::function<void()> m_OnAccepted;
    std::function<void()> m_OnDeclined;
    std::function<void()> m_PendingCallback;

    // DPI
    float m_DpiScale = 1.0f;
};

bool HasAcceptedFirstRunAgreement();
void SetAcceptedFirstRunAgreement(bool accepted);
std::string LoadFirstRunAgreementContent();
std::string LoadFirstRunAgreementEulaContent();
std::string LoadFirstRunAgreementThirdPartyNoticesContent();

} // namespace we::programs::editor

