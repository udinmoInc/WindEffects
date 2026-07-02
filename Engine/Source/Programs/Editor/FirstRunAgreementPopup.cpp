#include "FirstRunAgreementPopup.hpp"
#include "Core/EditorConfigPaths.hpp"
#include "Core/Icon.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_keycode.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace we::programs::editor {

namespace {

constexpr float kDialogPadding = 18.0f;
constexpr float kButtonHeight = 30.0f;
constexpr float kButtonWidth = 108.0f;
constexpr float kButtonGap = 10.0f;
constexpr float kCopyButtonWidth = 112.0f;
constexpr float kContentTopGap = 50.0f;
constexpr float kContentBottomGap = 56.0f;
constexpr float kLineGap = 6.0f;

std::filesystem::path GetAgreementStatePath() {
    return we::core::ResolveEditorConfigPath("first_run_agreement.ini");
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::filesystem::path ResolveRuntimeFile(const std::string& relativePath) {
    const std::filesystem::path p(relativePath);
    if (std::filesystem::exists(p)) return p;
    const std::filesystem::path candidates[] = {
        std::filesystem::path("..") / relativePath,
        std::filesystem::path("../..") / relativePath
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) return candidate;
    }
    return {};
}

} // namespace

FirstRunAgreementPopup::FirstRunAgreementPopup(std::string markdownAndTextContent)
    : m_RawContent(std::move(markdownAndTextContent))
{
    if (m_RawContent.empty()) {
        m_RawContent = BuildDefaultAgreementContent();
    }
    m_Title = ExtractTitleFromMarkdown(m_RawContent);
    m_AgreeButton.label = "I Agree";
    m_AgreeButton.iconName = we::UI::Icons::CheckName;
    m_DeclineButton.label = "Quit";
    m_DeclineButton.iconName = we::UI::Icons::XName;
    m_CopyButton.label = "Copy";
    m_CopyButton.iconName = we::UI::Icons::CopyName;
}

void FirstRunAgreementPopup::SetViewportSize(float width, float height) {
    m_ViewportWidth = std::max(320.0f, width);
    m_ViewportHeight = std::max(240.0f, height);
}

we::UI::Size FirstRunAgreementPopup::Measure(const we::UI::Size& availableSize) {
    // OverlayManager measures popups with a very large "availableSize".
    // Clamp to actual viewport size so the popup dialog stays on-screen.
    const float w = std::min((availableSize.width > 0.0f) ? availableSize.width : m_ViewportWidth, m_ViewportWidth);
    const float h = std::min((availableSize.height > 0.0f) ? availableSize.height : m_ViewportHeight, m_ViewportHeight);
    m_DesiredSize = we::UI::Size{ std::max(320.0f, w), std::max(240.0f, h) };
    return m_DesiredSize;
}

void FirstRunAgreementPopup::Arrange(const we::UI::Rect& allottedRect) {
    m_Geometry = allottedRect;

    const float dialogW = std::min(860.0f, std::max(560.0f, allottedRect.width - 60.0f));
    const float dialogH = std::min(620.0f, std::max(420.0f, allottedRect.height - 60.0f));
    m_DialogRect = we::UI::Rect{
        allottedRect.x + (allottedRect.width - dialogW) * 0.5f,
        allottedRect.y + (allottedRect.height - dialogH) * 0.5f,
        dialogW,
        dialogH
    };

    m_ContentRect = we::UI::Rect{
        m_DialogRect.x + kDialogPadding,
        m_DialogRect.y + kContentTopGap,
        m_DialogRect.width - kDialogPadding * 2.0f,
        m_DialogRect.height - (kContentTopGap + kContentBottomGap)
    };

    const float buttonY = m_DialogRect.y + m_DialogRect.height - kDialogPadding - kButtonHeight;
    m_AgreeButton.rect = we::UI::Rect{
        m_DialogRect.x + m_DialogRect.width - kDialogPadding - kButtonWidth,
        buttonY,
        kButtonWidth,
        kButtonHeight
    };
    m_DeclineButton.rect = we::UI::Rect{
        m_AgreeButton.rect.x - kButtonGap - kButtonWidth,
        buttonY,
        kButtonWidth,
        kButtonHeight
    };
    m_CopyButton.rect = we::UI::Rect{
        m_DialogRect.x + m_DialogRect.width - kDialogPadding - kCopyButtonWidth,
        m_DialogRect.y + 12.0f,
        kCopyButtonWidth,
        26.0f
    };
}

void FirstRunAgreementPopup::Paint(we::UI::PaintContext& context) {
    const auto& theme = we::UI::Theme::Get();

    context.DrawRect(m_Geometry, we::UI::Color{ 0.0f, 0.0f, 0.0f, 0.58f });
    context.DrawShadow(m_DialogRect, we::UI::Color{ 0.0f, 0.0f, 0.0f, 0.32f }, 8.0f, 14.0f);
    context.DrawRoundedRect(m_DialogRect, theme.PanelBackground, 8.0f);
    context.DrawRoundedRectOutline(m_DialogRect, theme.BorderDefault, 1.0f, 8.0f);

    context.DrawText(m_Title.empty() ? "License Agreement" : m_Title, we::UI::Point{ m_DialogRect.x + kDialogPadding, m_DialogRect.y + 14.0f },
        theme.TextPrimary, 16.0f, true);
    context.DrawText("Please review and accept before using the editor.",
        we::UI::Point{ m_DialogRect.x + kDialogPadding, m_DialogRect.y + 34.0f },
        theme.TextSecondary, 11.0f);
    context.DrawText("Press Ctrl+C to copy the full agreement text.",
        we::UI::Point{ m_DialogRect.x + kDialogPadding, m_DialogRect.y + m_DialogRect.height - 36.0f },
        theme.TextSecondary, 10.0f);

    context.DrawRoundedRect(m_ContentRect, theme.WorkspaceBackground, 4.0f);
    context.DrawRoundedRectOutline(m_ContentRect, theme.BorderDefault, 1.0f, 4.0f);
    context.PushClipRect(m_ContentRect);

    RebuildDisplayLines(context, m_ContentRect.width - 14.0f);
    float y = m_ContentRect.y + 8.0f - m_ScrollOffset;
    for (const auto& line : m_DisplayLines) {
        if (y + line.fontSize >= m_ContentRect.y && y <= m_ContentRect.y + m_ContentRect.height) {
            context.DrawText(line.text, we::UI::Point{ m_ContentRect.x + 7.0f, y }, line.color, line.fontSize, line.bold);
        }
        y += line.fontSize + kLineGap;
    }

    context.PopClipRect();

    // Scrollbar
    if (m_TotalContentHeight > m_ContentRect.height + 1.0f) {
        const float trackH = m_ContentRect.height - 8.0f;
        const float trackY = m_ContentRect.y + 4.0f;
        const float thumbH = std::max(20.0f, trackH * (m_ContentRect.height / m_TotalContentHeight));
        const float scrollRange = std::max(1.0f, m_TotalContentHeight - m_ContentRect.height);
        const float thumbY = trackY + (trackH - thumbH) * (m_ScrollOffset / scrollRange);
        const we::UI::Rect track{ m_ContentRect.x + m_ContentRect.width - 6.0f, trackY, 3.0f, trackH };
        const we::UI::Rect thumb{ track.x, thumbY, 3.0f, thumbH };
        context.DrawRoundedRect(track, we::UI::Color{ 1.0f, 1.0f, 1.0f, 0.10f }, 2.0f);
        context.DrawRoundedRect(thumb, we::UI::Color{ 1.0f, 1.0f, 1.0f, 0.36f }, 2.0f);
    }

    auto paintButton = [&](const ButtonState& button, bool primary) {
        we::UI::Color bg = primary ? theme.SelectedAccent : theme.HoverOverlay;
        if (!button.hovered) bg.a *= 0.85f;
        if (button.pressed) bg = we::UI::Color::Lerp(bg, theme.PressedOverlay, 0.45f);
        context.DrawRoundedRect(button.rect, bg, 4.0f);
        context.DrawRoundedRectOutline(button.rect, theme.BorderDefault, 1.0f, 4.0f);
        const float iconSize = 12.0f;
        const float textGap = button.iconName.empty() ? 0.0f : 8.0f;
        const float tw = context.GetTextWidth(button.label, 12.0f);
        const float contentW = tw + (button.iconName.empty() ? 0.0f : iconSize + textGap);
        float contentX = button.rect.x + (button.rect.width - contentW) * 0.5f;
        const float contentY = button.rect.y + (button.rect.height - iconSize) * 0.5f;
        const we::UI::Color textColor = primary ? we::UI::Color{ 0.10f, 0.10f, 0.10f, 1.0f } : theme.TextPrimary;
        if (!button.iconName.empty()) {
            we::UI::IconPainter::DrawIcon(context, button.iconName,
                we::UI::Rect{ contentX, contentY, iconSize, iconSize }, textColor);
            contentX += iconSize + textGap;
        }
        context.DrawText(button.label,
            we::UI::Point{ contentX, button.rect.y + 8.0f }, textColor, 12.0f, true);
    };

    paintButton(m_CopyButton, false);
    paintButton(m_DeclineButton, false);
    paintButton(m_AgreeButton, true);
}

void FirstRunAgreementPopup::OnMouseDown(const we::UI::MouseEvent& event) {
    if (event.button != we::UI::MouseButton::Left) return;
    m_AgreeButton.pressed = m_AgreeButton.rect.Contains(event.position);
    m_DeclineButton.pressed = m_DeclineButton.rect.Contains(event.position);
    m_CopyButton.pressed = m_CopyButton.rect.Contains(event.position);
}

void FirstRunAgreementPopup::OnMouseMove(const we::UI::MouseEvent& event) {
    m_AgreeButton.hovered = m_AgreeButton.rect.Contains(event.position);
    m_DeclineButton.hovered = m_DeclineButton.rect.Contains(event.position);
    m_CopyButton.hovered = m_CopyButton.rect.Contains(event.position);
}

void FirstRunAgreementPopup::OnMouseUp(const we::UI::MouseEvent& event) {
    if (event.button != we::UI::MouseButton::Left) return;

    const bool acceptClicked = m_AgreeButton.pressed && m_AgreeButton.rect.Contains(event.position);
    const bool declineClicked = m_DeclineButton.pressed && m_DeclineButton.rect.Contains(event.position);
    const bool copyClicked = m_CopyButton.pressed && m_CopyButton.rect.Contains(event.position);
    m_AgreeButton.pressed = false;
    m_DeclineButton.pressed = false;
    m_CopyButton.pressed = false;

    if (copyClicked) SDL_SetClipboardText(m_RawContent.c_str());
    if (acceptClicked && m_OnAccepted) m_OnAccepted();
    if (declineClicked && m_OnDeclined) m_OnDeclined();
}

void FirstRunAgreementPopup::OnMouseWheel(const we::UI::MouseEvent& event) {
    if (!m_DialogRect.Contains(event.position)) return;
    const float maxScroll = std::max(0.0f, m_TotalContentHeight - m_ContentRect.height);
    m_ScrollOffset = std::clamp(m_ScrollOffset - event.wheelDeltaY * 36.0f, 0.0f, maxScroll);
}

void FirstRunAgreementPopup::OnKeyDown(const we::UI::KeyEvent& event) {
    if (event.ctrlDown && event.keycode == SDLK_C) {
        SDL_SetClipboardText(m_RawContent.c_str());
        return;
    }
    if (event.keycode == SDLK_ESCAPE) {
        if (m_OnDeclined) m_OnDeclined();
    }
}

bool FirstRunAgreementPopup::ShowsPointerCursor(const we::UI::Point& position) const {
    return m_AgreeButton.rect.Contains(position) || m_DeclineButton.rect.Contains(position) || m_CopyButton.rect.Contains(position);
}

void FirstRunAgreementPopup::RebuildDisplayLines(we::UI::PaintContext& context, float maxWidth) {
    if (std::abs(maxWidth - m_CachedWrapWidth) < 0.5f && !m_DisplayLines.empty()) return;

    m_CachedWrapWidth = maxWidth;
    m_DisplayLines.clear();

    std::istringstream input(m_RawContent);
    std::string line;
    bool inCodeBlock = false;
    while (std::getline(input, line)) {
        if (line.rfind("```", 0) == 0) {
            inCodeBlock = !inCodeBlock;
            continue;
        }

        DisplayLine style;
        style.fontSize = 12.0f;
        style.bold = false;
        style.color = we::UI::Theme::Get().TextSecondary;

        std::string normalized = line;
        if (inCodeBlock) {
            style.fontSize = 11.0f;
            style.color = we::UI::Theme::Get().TextPrimary;
        } else if (line.rfind("# ", 0) == 0) {
            normalized = line.substr(2);
            style.fontSize = 15.0f;
            style.bold = true;
            style.color = we::UI::Theme::Get().TextPrimary;
        } else if (line.rfind("## ", 0) == 0) {
            normalized = line.substr(3);
            style.fontSize = 13.0f;
            style.bold = true;
            style.color = we::UI::Theme::Get().TextPrimary;
        } else if (line.rfind("### ", 0) == 0) {
            normalized = line.substr(4);
            style.fontSize = 12.0f;
            style.bold = true;
            style.color = we::UI::Theme::Get().TextPrimary;
        } else if (line.rfind("- ", 0) == 0 || line.rfind("* ", 0) == 0) {
            normalized = "  - " + line.substr(2);
            style.color = we::UI::Theme::Get().TextPrimary;
        } else if (line.empty()) {
            DisplayLine spacer;
            spacer.text = "";
            spacer.fontSize = 10.0f;
            spacer.color = we::UI::Color::Transparent();
            m_DisplayLines.push_back(spacer);
            continue;
        } else {
            style.color = we::UI::Theme::Get().TextPrimary;
        }

        normalized = NormalizeMarkdownLine(normalized);
        const auto wrapped = WrapLine(context, normalized, style.fontSize, maxWidth);
        if (wrapped.empty()) {
            m_DisplayLines.push_back(style);
            m_DisplayLines.back().text = normalized;
        } else {
            for (const auto& chunk : wrapped) {
                DisplayLine copy = style;
                copy.text = chunk;
                m_DisplayLines.push_back(copy);
            }
        }
    }

    m_TotalContentHeight = 16.0f;
    for (const auto& item : m_DisplayLines) {
        m_TotalContentHeight += item.fontSize + kLineGap;
    }
    const float maxScroll = std::max(0.0f, m_TotalContentHeight - m_ContentRect.height);
    m_ScrollOffset = std::clamp(m_ScrollOffset, 0.0f, maxScroll);
}

std::vector<std::string> FirstRunAgreementPopup::WrapLine(
    we::UI::PaintContext& context, const std::string& text, float fontSize, float maxWidth) const {
    if (text.empty()) return {};
    if (context.GetTextWidth(text, fontSize) <= maxWidth) return { text };

    std::istringstream words(text);
    std::string word;
    std::string current;
    std::vector<std::string> wrapped;

    while (words >> word) {
        std::string candidate = current.empty() ? word : current + " " + word;
        if (context.GetTextWidth(candidate, fontSize) <= maxWidth) {
            current = candidate;
        } else {
            if (!current.empty()) wrapped.push_back(current);
            current = word;
        }
    }
    if (!current.empty()) wrapped.push_back(current);
    return wrapped;
}

std::string FirstRunAgreementPopup::ExtractTitleFromMarkdown(const std::string& content) {
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("# ", 0) == 0) {
            return line.substr(2);
        }
        if (line.rfind("## ", 0) == 0) {
            return line.substr(3);
        }
        if (line.rfind("### ", 0) == 0) {
            return line.substr(4);
        }
    }
    return "License Agreement";
}

std::string FirstRunAgreementPopup::NormalizeMarkdownLine(const std::string& line) {
    std::string out = line;
    auto replaceAll = [](std::string& value, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = value.find(from, pos)) != std::string::npos) {
            value.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replaceAll(out, "**", "");
    replaceAll(out, "__", "");
    replaceAll(out, "`", "");
    return out;
}

std::string FirstRunAgreementPopup::BuildDefaultAgreementContent() {
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

bool HasAcceptedFirstRunAgreement() {
    const auto path = GetAgreementStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line == "accepted=1" || line == "accepted=true") return true;
    }
    return false;
}

void SetAcceptedFirstRunAgreement(bool accepted) {
    const auto path = GetAgreementStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) return;
    file << "accepted=" << (accepted ? "1" : "0") << "\n";
}

std::string LoadFirstRunAgreementContent() {
    const std::string eula = LoadFirstRunAgreementEulaContent();
    const std::string thirdParty = LoadFirstRunAgreementThirdPartyNoticesContent();

    if (eula.empty() && thirdParty.empty()) {
        return FirstRunAgreementPopup::BuildDefaultAgreementContent();
    }

    if (eula.empty()) return thirdParty;
    if (thirdParty.empty()) return eula;

    return eula + "\n\n" + thirdParty;
}

std::string LoadFirstRunAgreementEulaContent() {
    const auto eulaPath = ResolveRuntimeFile("Legal/EULA.md");
    std::string eula = eulaPath.empty() ? std::string{} : ReadTextFile(eulaPath);
    if (eula.empty()) {
        return
            "# End User License Agreement\n\n"
            "License text was not found. Please review the project documentation.\n";
    }
    return std::string("# End User License Agreement\n\n") + eula;
}

std::string LoadFirstRunAgreementThirdPartyNoticesContent() {
    const auto noticesPath = ResolveRuntimeFile("Legal/THIRD_PARTY_NOTICES.md");
    std::string notices = noticesPath.empty() ? std::string{} : ReadTextFile(noticesPath);
    if (notices.empty()) {
        return
            "# Third-Party Notices\n\n"
            "Third-party notice text was not found. Please review the project documentation.\n";
    }
    return std::string("# Third-Party Notices\n\n") + notices;
}

} // namespace we::programs::editor

