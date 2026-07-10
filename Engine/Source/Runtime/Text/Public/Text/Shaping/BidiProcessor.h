#pragma once

#include "Text/Shaping/TextShaper.h"

#include <span>
#include <vector>

namespace we::runtime::text::shaping {

struct VisualRun {
    std::span<const Codepoint> codepoints;
    TextDirection direction = TextDirection::LeftToRight;
    size_t startIndex = 0;
    size_t length = 0;
};

class TEXT_API IBidiProcessor {
public:
    virtual ~IBidiProcessor() = default;
    [[nodiscard]] virtual std::vector<VisualRun> ReorderVisual(
        std::span<const Codepoint> codepoints) const = 0;
};

[[nodiscard]] TEXT_API std::unique_ptr<IBidiProcessor> CreateBidiProcessor();

} // namespace we::runtime::text::shaping
