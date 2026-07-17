#pragma once

#include "Text/Core/Types.h"
#include "Text/Export.h"
#include "Text/Unicode/UnicodeDecoder.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::text::unicode {

/// Minimal UAX#29-inspired grapheme cluster boundaries (no ICU).
[[nodiscard]] TEXT_API bool IsGraphemeBreak(Codepoint prev, Codepoint next);

struct GraphemeRange {
    size_t start = 0; // codepoint index
    size_t end = 0;   // exclusive
};

[[nodiscard]] TEXT_API std::vector<GraphemeRange> SegmentGraphemes(
    std::span<const Codepoint> codepoints);

[[nodiscard]] TEXT_API size_t NextGraphemeOffset(
    std::span<const Codepoint> codepoints,
    size_t codepointIndex);

[[nodiscard]] TEXT_API size_t PrevGraphemeOffset(
    std::span<const Codepoint> codepoints,
    size_t codepointIndex);

[[nodiscard]] TEXT_API std::vector<Codepoint> DecodeUtf8(std::string_view utf8);
[[nodiscard]] TEXT_API std::string EncodeUtf8(std::span<const Codepoint> codepoints);

} // namespace we::runtime::text::unicode
