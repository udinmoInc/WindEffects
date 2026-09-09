// ==============================================================================
// WindEffects — Text — Grapheme
// Public API surface for the Text module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
    size_t start = 0;
    size_t end = 0;
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
