#include "Text/Unicode/Grapheme.h"

namespace we::runtime::text::unicode {
namespace {

bool IsCombiningMark(const Codepoint cp)
{
    return (cp >= 0x0300 && cp <= 0x036F)
        || (cp >= 0x1AB0 && cp <= 0x1AFF)
        || (cp >= 0x1DC0 && cp <= 0x1DFF)
        || (cp >= 0x20D0 && cp <= 0x20FF)
        || (cp >= 0xFE20 && cp <= 0xFE2F);
}

bool IsEmojiModifier(const Codepoint cp)
{
    return cp >= 0x1F3FB && cp <= 0x1F3FF;
}

bool IsRegionalIndicator(const Codepoint cp)
{
    return cp >= 0x1F1E6 && cp <= 0x1F1FF;
}

} // namespace

bool IsGraphemeBreak(const Codepoint prev, const Codepoint next)
{
    if (next == 0x200D) { // ZWJ
        return false;
    }
    if (prev == 0x200D) {
        return false;
    }
    if (IsCombiningMark(next) || IsEmojiModifier(next)) {
        return false;
    }
    if (IsRegionalIndicator(prev) && IsRegionalIndicator(next)) {
        return false;
    }
    if (next == 0xFE0F) { // variation selector
        return false;
    }
    return true;
}

std::vector<GraphemeRange> SegmentGraphemes(const std::span<const Codepoint> codepoints)
{
    std::vector<GraphemeRange> ranges;
    if (codepoints.empty()) {
        return ranges;
    }
    size_t start = 0;
    for (size_t i = 1; i < codepoints.size(); ++i) {
        if (IsGraphemeBreak(codepoints[i - 1], codepoints[i])) {
            ranges.push_back({start, i});
            start = i;
        }
    }
    ranges.push_back({start, codepoints.size()});
    return ranges;
}

size_t NextGraphemeOffset(const std::span<const Codepoint> codepoints, const size_t codepointIndex)
{
    const auto ranges = SegmentGraphemes(codepoints);
    for (const auto& range : ranges) {
        if (range.start >= codepointIndex && range.end > codepointIndex) {
            return range.end;
        }
        if (range.start > codepointIndex) {
            return range.start;
        }
    }
    return codepoints.size();
}

size_t PrevGraphemeOffset(const std::span<const Codepoint> codepoints, const size_t codepointIndex)
{
    if (codepointIndex == 0 || codepoints.empty()) {
        return 0;
    }
    const auto ranges = SegmentGraphemes(codepoints);
    for (auto it = ranges.rbegin(); it != ranges.rend(); ++it) {
        if (it->start < codepointIndex) {
            return it->start;
        }
    }
    return 0;
}

std::vector<Codepoint> DecodeUtf8(const std::string_view utf8)
{
    UnicodeDecoder decoder;
    const auto bytes = std::span(
        reinterpret_cast<const std::byte*>(utf8.data()),
        utf8.size());
    return decoder.Decode(bytes, Encoding::Utf8).codepoints;
}

std::string EncodeUtf8(const std::span<const Codepoint> codepoints)
{
    std::string out;
    out.reserve(codepoints.size());
    for (const Codepoint cp : codepoints) {
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

} // namespace we::runtime::text::unicode
