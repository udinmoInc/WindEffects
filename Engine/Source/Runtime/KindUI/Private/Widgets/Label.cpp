// ==============================================================================
// WindEffects — KindUI — Label
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/Label.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Text/Layout/TextStyle.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {
namespace {

std::string EllipsizeToWidth(
    const std::string& text,
    const float fontSize,
    const bool bold,
    const float maxWidth) {
    if (maxWidth <= 0.0f || text.empty()) {
        return text;
    }
    if (TextMetrics::MeasureWidth(text, fontSize, bold) <= maxWidth) {
        return text;
    }

    constexpr const char* kEllipsis = "...";
    const float ellipsisWidth = TextMetrics::MeasureWidth(kEllipsis, fontSize, bold);
    if (ellipsisWidth >= maxWidth) {
        return kEllipsis;
    }
    const float targetWidth = maxWidth - ellipsisWidth;

    size_t low = 0;
    size_t high = text.size();
    size_t bestLen = 0;

    while (low <= high) {
        const size_t mid = low + (high - low) / 2;
        std::string_view sub(text.data(), mid);
        if (TextMetrics::MeasureWidth(sub, fontSize, bold) <= targetWidth) {
            bestLen = mid;
            low = mid + 1;
        } else {
            if (mid == 0) break;
            high = mid - 1;
        }
    }

    std::string result;
    result.reserve(bestLen + 3);
    result.assign(text.data(), bestLen);
    result += kEllipsis;
    return result;
}

void SplitLines(const std::string& text, std::vector<std::string>& out) {
    out.clear();
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            out.push_back(text.substr(start));
            break;
        }
        out.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    if (out.empty()) {
        out.emplace_back();
    }
}

} // namespace

Label::Label(const std::string& text, TypographyToken role)
    : m_Text(text)
    , m_Style(TextStyle::FromRole(role))
{
    // Force neutral colors for label roles to prevent any green tint
    if (role == TypographyToken::PropertyLabel || role == TypographyToken::Caption || role == TypographyToken::Error) {
        m_Style.color = ResolveColor(ColorToken::TextSecondary);
    }
}

Label::Label(const std::string& text, const Color& color, float fontSize)
    : m_Text(text)
    , m_Style(TextStyle::Body())
{
    // Default ctor args (White / 14) map to theme Body role instead of hardcoded white.
    const bool useThemeColor =
        color.r >= 0.999f && color.g >= 0.999f && color.b >= 0.999f && color.a >= 0.999f;
    const bool useThemeSize = fontSize <= 14.001f && fontSize >= 13.999f;
    if (!useThemeColor) {
        m_Style.color = color;
    }
    if (!useThemeSize) {
        m_Style.size = fontSize;
    }
}

float Label::LineHeight() const
{
    const TypographySpec spec = ResolveTypography(m_Style.role);
    if (spec.lineHeightPx > 0.0f && spec.sizePx > 0.0f) {
        if (std::abs(m_Style.size - spec.sizePx) < 0.01f) {
            return spec.lineHeightPx;
        }
        return m_Style.size * (spec.lineHeightPx / spec.sizePx);
    }
    return m_Style.size * 1.25f;
}

void Label::RebuildLines(float availableWidth) {
    m_WrappedLines.clear();

    if (m_WrapText && availableWidth > 0.0f) {
        size_t i = 0;
        std::string currentLine;
        while (i < m_Text.size()) {
            while (i < m_Text.size() && (m_Text[i] == ' ' || m_Text[i] == '\t' || m_Text[i] == '\r')) {
                ++i;
            }
            if (i >= m_Text.size()) {
                break;
            }
            if (m_Text[i] == '\n') {
                m_WrappedLines.push_back(currentLine);
                currentLine.clear();
                ++i;
                continue;
            }
            const size_t wordStart = i;
            while (i < m_Text.size() && m_Text[i] != ' ' && m_Text[i] != '\t' && m_Text[i] != '\n' && m_Text[i] !=
                '\r') {
                ++i;
            }
            const std::string_view word(m_Text.data() + wordStart, i - wordStart);
            if (currentLine.empty()) {
                currentLine.assign(word);
            } else {
                std::string candidate;
                candidate.reserve(currentLine.size() + 1 + word.size());
                candidate = currentLine;
                candidate.push_back(' ');
                candidate.append(word);
                if (TextMetrics::MeasureWidth(candidate, m_Style.size, m_Style.bold) > availableWidth) {
                    m_WrappedLines.push_back(std::move(currentLine));
                    currentLine.assign(word);
                } else {
                    currentLine = std::move(candidate);
                }
            }
        }
        if (!currentLine.empty() || m_WrappedLines.empty()) {
            m_WrappedLines.push_back(std::move(currentLine));
        }
    } else {
        SplitLines(m_Text, m_WrappedLines);
        if (availableWidth > 0.0f) {
            for (auto& wrapped : m_WrappedLines) {
                wrapped = EllipsizeToWidth(wrapped, m_Style.size, m_Style.bold, availableWidth);
            }
        }
    }

    m_CachedLayoutWidth = availableWidth;
    m_LinesCacheValid = true;
}

Size Label::Measure(const Size& availableSize) {
    const float layoutWidth = availableSize.width > 0.0f && availableSize.width < 1.0e8f
        ? availableSize.width
        : -1.0f;
    if (!m_LinesCacheValid || m_CachedLayoutWidth != layoutWidth) {
        RebuildLines(layoutWidth);
    }

    float maxWidth = 0.0f;
    for (const auto& line : m_WrappedLines) {
        const float lineWidth = TextMetrics::MeasureWidth(line, m_Style.size, m_Style.bold);
        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
    }

    const float lineHeight = LineHeight();
    const float height = static_cast<float>(std::max<size_t>(m_WrappedLines.size(), 1)) * lineHeight;
    if (layoutWidth > 0.0f && !m_WrapText) {
        maxWidth = std::min(maxWidth, layoutWidth);
    }
    m_DesiredSize = Size{ maxWidth, height };
    return m_DesiredSize;
}

void Label::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    // Re-ellipsize only when final arranged width differs from the Measure cache key.
    if (!m_WrapText && allottedRect.width > 0.0f) {
        if (!m_LinesCacheValid || m_CachedLayoutWidth != allottedRect.width) {
            RebuildLines(allottedRect.width);
        }
    }
}

void Label::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    const float lineHeight = LineHeight();
    const float contentH = static_cast<float>(std::max<size_t>(m_WrappedLines.size(), 1)) * lineHeight;
    float currentY = m_Geometry.y + std::max(0.0f, (m_Geometry.height - contentH) * 0.5f);

    for (const auto& line : m_WrappedLines) {
        context.DrawText(
            line,
            Point{ m_Geometry.x, currentY + (lineHeight - m_Style.size) * 0.5f },
            m_Style.color,
            m_Style.size,
            static_cast<we::runtime::text::layout::FontWeight>(
                m_Style.weight > 0 ? m_Style.weight : (m_Style.bold ? 600 : 400)),
            m_Style.italic);
        currentY += lineHeight;
    }
}

}
