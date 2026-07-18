#pragma once

#include "KindUI/Core/Widget.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace we::programs::editor {

class FirstRunAgreementPopup : public we::runtime::kindui::Widget {
public:
    explicit FirstRunAgreementPopup(std::string markdownAndTextContent);
    ~FirstRunAgreementPopup();

    void SetOnAccepted(std::function<void()> callback) { m_OnAccepted = std::move(callback); }
    void SetOnDeclined(std::function<void()> callback) { m_OnDeclined = std::move(callback); }

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseWheel(const we::runtime::kindui::MouseEvent& event) override;
    void OnKeyDown(const we::runtime::kindui::KeyEvent& event) override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;
    
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
        we::runtime::kindui::Rect rect{};
        std::string label;
        std::string iconName;
        bool hovered = false;
        bool pressed = false;
    };

    struct ScrollbarState {
        we::runtime::kindui::Rect track{};
        we::runtime::kindui::Rect thumb{};
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
    void ComputeLayout(we::runtime::kindui::PaintContext& context);
    float MeasureNode(we::runtime::kindui::PaintContext& context, DocumentNode& node, float maxWidth);
    std::vector<std::string> WrapWords(we::runtime::kindui::PaintContext& context, const std::string& text, float fontSize, float maxWidth);

    // Rendering
    void RenderNode(we::runtime::kindui::PaintContext& context, const DocumentNode& node, float y);
    void RenderTextRuns(we::runtime::kindui::PaintContext& context, const std::vector<TextRun>& runs, float x, float& y, float fontSize, const we::runtime::kindui::Color& baseColor, float maxWidth);
    void RenderCodeBlock(we::runtime::kindui::PaintContext& context, const DocumentNode& node, float y);
    void RenderBlockquote(we::runtime::kindui::PaintContext& context, const DocumentNode& node, float y);
    void RenderList(we::runtime::kindui::PaintContext& context, const DocumentNode& node, float y);

    // Scrolling
    void SetScrollOffset(float offset);
    void ScrollBy(float delta);
    void ScrollPageUp();
    void ScrollPageDown();
    void ScrollToTop();
    void ScrollToBottom();
    void UpdateScrollbarGeometry();
    bool IsOverScrollbar(const we::runtime::kindui::Point& point) const;
    bool IsOverThumb(const we::runtime::kindui::Point& point) const;

    // Utilities
    float GetFontSize(NodeType type) const;
    float GetLineHeight(NodeType type) const;
    we::runtime::kindui::Color GetTextColor(NodeType type) const;

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
    we::runtime::kindui::Rect m_DialogRect{};
    we::runtime::kindui::Rect m_ContentRect{};
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

