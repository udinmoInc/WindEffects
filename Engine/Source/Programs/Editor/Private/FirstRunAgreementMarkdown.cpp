#include "Platform/Platform.h"
#include "FirstRunAgreementPopup.h"
#include "FirstRunAgreementInternal.h"
#include "Core/EditorConfigPaths.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/DPIContext.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>

namespace we::programs::editor {
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MouseButton;
namespace Icons = ::we::runtime::kindui::Icons;
using namespace first_run_detail;

using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
void FirstRunAgreementPopup::ParseDocument() {
    m_DocumentNodes.clear();
    
    std::istringstream input(m_RawContent);
    std::string line;
    bool inCodeBlock = false;
    std::string codeBlockContent;
    
    while (std::getline(input, line)) {
        // Code blocks
        if (StartsWith(line, "```")) {
            if (inCodeBlock) {
                DocumentNode node;
                node.type = NodeType::CodeBlock;
                node.rawText = codeBlockContent;
                node.marginTop = kParagraphSpacing * m_DpiScale;
                node.marginBottom = kParagraphSpacing * m_DpiScale;
                m_DocumentNodes.push_back(node);
                codeBlockContent.clear();
            }
            inCodeBlock = !inCodeBlock;
            continue;
        }
        
        if (inCodeBlock) {
            codeBlockContent += line + "\n";
            continue;
        }
        
        const std::string trimmed = Trim(line);
        
        // Horizontal rule
        if (trimmed == "---" || trimmed == "***" || StartsWith(trimmed, "---") || StartsWith(trimmed, "***")) {
            DocumentNode node;
            node.type = NodeType::HorizontalRule;
            node.marginTop = kParagraphSpacing * m_DpiScale;
            node.marginBottom = kParagraphSpacing * m_DpiScale;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Blockquotes
        if (StartsWith(trimmed, ">")) {
            DocumentNode node;
            node.type = NodeType::Blockquote;
            node.runs = ParseInlineText(TrimLeft(trimmed.substr(1)));
            node.marginTop = kParagraphSpacing * m_DpiScale;
            node.marginBottom = kParagraphSpacing * m_DpiScale;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Empty lines
        if (trimmed.empty()) {
            DocumentNode node;
            node.type = NodeType::Spacer;
            node.marginTop = kParagraphSpacing * m_DpiScale * 0.5f;
            node.marginBottom = kParagraphSpacing * m_DpiScale * 0.5f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Headings
        if (StartsWith(trimmed, "# ")) {
            DocumentNode node;
            node.type = NodeType::Heading1;
            node.runs = ParseInlineText(trimmed.substr(2));
            node.marginTop = kHeadingSpacing * m_DpiScale;
            node.marginBottom = kParagraphSpacing * m_DpiScale * 0.5f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        if (StartsWith(trimmed, "## ")) {
            DocumentNode node;
            node.type = NodeType::Heading2;
            node.runs = ParseInlineText(trimmed.substr(3));
            node.marginTop = kHeadingSpacing * m_DpiScale * 0.8f;
            node.marginBottom = kParagraphSpacing * m_DpiScale * 0.4f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        if (StartsWith(trimmed, "### ")) {
            DocumentNode node;
            node.type = NodeType::Heading3;
            node.runs = ParseInlineText(trimmed.substr(4));
            node.marginTop = kHeadingSpacing * m_DpiScale * 0.6f;
            node.marginBottom = kParagraphSpacing * m_DpiScale * 0.3f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Ordered lists
        static const std::regex orderedListRegex(R"(^(\d+)[.)]\s+(.*)$)");
        std::smatch orderedMatch;
        if (std::regex_match(trimmed, orderedMatch, orderedListRegex)) {
            DocumentNode node;
            node.type = NodeType::OrderedList;
            node.listNumber = std::stoi(orderedMatch[1].str());
            node.runs = ParseInlineText(orderedMatch[2].str());
            node.marginTop = kListSpacing * m_DpiScale;
            node.marginBottom = kListSpacing * m_DpiScale * 0.25f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Unordered lists
        if (StartsWith(trimmed, "- ") || StartsWith(trimmed, "* ") || StartsWith(trimmed, "+ ")) {
            DocumentNode node;
            node.type = NodeType::UnorderedList;
            node.runs = ParseInlineText(trimmed.substr(2));
            node.marginTop = kListSpacing * m_DpiScale;
            node.marginBottom = kListSpacing * m_DpiScale * 0.25f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Paragraph
        DocumentNode node;
        node.type = NodeType::Paragraph;
        node.runs = ParseInlineText(trimmed);
        node.marginTop = kParagraphSpacing * m_DpiScale;
        node.marginBottom = kParagraphSpacing * m_DpiScale;
        m_DocumentNodes.push_back(node);
    }
}

std::vector<FirstRunAgreementPopup::TextRun> FirstRunAgreementPopup::ParseInlineText(const std::string& text) {
    std::vector<TextRun> runs;
    std::string current;
    bool inBold = false;
    bool inItalic = false;
    bool inCode = false;
    
    for (size_t i = 0; i < text.length(); ++i) {
        // Bold: **text**
        if (i + 1 < text.length() && text[i] == '*' && text[i + 1] == '*') {
            if (!current.empty()) {
                TextRun run;
                run.text = current;
                run.style = inCode ? TextStyle::Code : (inBold ? TextStyle::Bold : (inItalic ? TextStyle::Italic : TextStyle::Normal));
                runs.push_back(run);
                current.clear();
            }
            inBold = !inBold;
            i += 1;
            continue;
        }
        
        // Italic: *text* or _text_
        if ((text[i] == '*' || text[i] == '_') && !inBold) {
            if (!current.empty()) {
                TextRun run;
                run.text = current;
                run.style = inCode ? TextStyle::Code : (inItalic ? TextStyle::Italic : TextStyle::Normal);
                runs.push_back(run);
                current.clear();
            }
            inItalic = !inItalic;
            continue;
        }
        
        // Code: `text`
        if (text[i] == '`') {
            if (!current.empty()) {
                TextRun run;
                run.text = current;
                run.style = inCode ? TextStyle::Code : (inBold ? TextStyle::Bold : (inItalic ? TextStyle::Italic : TextStyle::Normal));
                runs.push_back(run);
                current.clear();
            }
            inCode = !inCode;
            continue;
        }
        
        // Links: [text](url)
        if (text[i] == '[' && !inCode) {
            size_t linkEnd = text.find(']', i);
            size_t urlStart = text.find('(', linkEnd);
            size_t urlEnd = text.find(')', urlStart);
            
            if (linkEnd != std::string::npos && urlStart != std::string::npos && urlEnd != std::string::npos) {
                if (!current.empty()) {
                    TextRun run;
                    run.text = current;
                    run.style = TextStyle::Normal;
                    runs.push_back(run);
                    current.clear();
                }
                
                TextRun linkRun;
                linkRun.text = text.substr(linkEnd + 1, urlStart - linkEnd - 1);
                linkRun.style = TextStyle::Link;
                linkRun.linkUrl = text.substr(urlStart + 1, urlEnd - urlStart - 1);
                runs.push_back(linkRun);
                
                i = urlEnd;
                continue;
            }
        }
        
        current += text[i];
    }
    
    if (!current.empty()) {
        TextRun run;
        run.text = current;
        run.style = inCode ? TextStyle::Code : (inBold ? TextStyle::Bold : (inItalic ? TextStyle::Italic : TextStyle::Normal));
        runs.push_back(run);
    }
    
    return runs;
}

std::string FirstRunAgreementPopup::StripMarkdown(const std::string& text) {
    std::string result = text;
    
    size_t pos = 0;
    while ((pos = result.find("**", pos)) != std::string::npos) {
        result.replace(pos, 2, "");
    }
    
    pos = 0;
    while ((pos = result.find("*", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    pos = 0;
    while ((pos = result.find("_", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    
    pos = 0;
    while ((pos = result.find("`", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    
    std::regex linkRegex(R"(\[([^\]]+)\]\([^\)]+\))");
    result = std::regex_replace(result, linkRegex, "$1");
    
    return result;
}

std::string FirstRunAgreementPopup::ExtractTitle(const std::string& content) {
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmed = Trim(line);
        if (StartsWith(trimmed, "# ")) {
            return StripMarkdown(trimmed.substr(2));
        }
        if (StartsWith(trimmed, "## ")) {
            return StripMarkdown(trimmed.substr(3));
        }
        if (StartsWith(trimmed, "### ")) {
            return StripMarkdown(trimmed.substr(4));
        }
    }
    return "License Agreement";
}


std::string FirstRunAgreementPopup::BuildDefaultContent() {
    return
        "# WindEffects Editor Terms\n"
        "\n"
        "By using this editor, you agree to the license and third-party notices.\n"
        "\n"
        "## Summary\n"
        "- Use this software according to the project license.\n"
        "- Review third-party notices and obligations.\n"
        "- If you do not agree, choose Quit.\n";
}

} // namespace we::programs::editor
